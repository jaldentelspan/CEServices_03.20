#ifndef __MITR_I2C_TYPES_H__
#define __MITR_I2C_TYPES_H__


#if defined (BOARD_MITR_REF)
#define DISCRETE_IN_MAX		4
#define DISCRETE_OUT_MAX	12
#define DISCRETE_OUT_MASK	0xFFF
#define MITR_PORT_COUNT		12
#define MODEL_NAME "iES-12"
#elif defined (BOARD_NOT_APPLICABLE_REF)
#define DISCRETE_IN_MAX		1
#define DISCRETE_OUT_MAX	8
#define DISCRETE_OUT_MASK	0xFF
#define MITR_PORT_COUNT		6
#define MODEL_NAME "iES-6"
#elif defined (BOARD_NCGU_REF)
#define DISCRETE_IN_MAX		4
#define DISCRETE_OUT_MAX	4
#define MITR_PORT_COUNT		6
#define MODEL_NAME "NCGU"
#endif

#define MITR_CONFIG_VERSION 	4
#define MITR_VERSION_MAJOR	1
#define MITR_VERSION_MINOR	2

#define MANUFACTURER_NAME "Telspan Data"

typedef enum{
	MITR_ERROR_INVALID_COMMAND = 0,
	MITR_ERROR_INVALID_PARAMETER = 1,
	MITR_ERROR_INVALID_MODE = 2,
	MITR_ERROR_NO_MEDIA = 3,
	MITR_ERROR_MEDIA_FULL = 4,
	MITR_ERROR_COMMAND_FAILED = 5
}mitr_fpga_error_code_t;

typedef enum{
	MITR_API_CMD_SUCCESS = 0,
	MITR_API_CMD_BUSY,
	MITR_API_CMD_FAIL
}mitr_api_cmd_err;

static char * MITR_ERROR_CODE_STR[] = {
	[MITR_ERROR_INVALID_COMMAND] = "Invalid Command",
	[MITR_ERROR_INVALID_PARAMETER] = "Invalid Parameter",
	[MITR_ERROR_INVALID_MODE] = "Invalid Mode",
	[MITR_ERROR_NO_MEDIA] = "No Media",
	[MITR_ERROR_MEDIA_FULL] = "Media Full",
	[MITR_ERROR_COMMAND_FAILED] = "Command Failed",
};

typedef enum{
	DIRECTION_NONE = 0,
	DIRECTION_IN,
	DIRECTION_OUT
}mitr_direction_t;

enum common_health_flags{
	HEALTH_BIT_FAIL 	= 0x01,
	HEALTH_SETUP_FAIL 	= 0x02
};

enum system_health_flags{
	SYS_HEALTH_OPERATION_FAIL 			= 0x04,
	SYS_HEALTH_BUSY 					= 0x08,
	SYS_HEALTH_POWER_BOARD_OVER_TEMP 	= 0x10,
	SYS_HEALTH_FPGA_BOARD_PSC_FAIL 		= 0x80,
	SYS_HEALTH_GPS_FAULT 				= 0x100
};

enum gps_health_flags{
	GPS_HEALTH_NO_ANTENNA_SIGNAL 		= 0x04,
	GPS_HEALTH_BAD_ANTENNA_SIGNAL 		= 0x08,
	GPS_HEALTH_TIME_SYNC_FAIL 			= 0x10,
	GPS_HEALTH_NO_SATELLITES 			= 0x20
};

enum time_input_health_flags{
	TI_HEALTH_NO_SIGNAL 				= 0x04,
	TI_HEALTH_BAD_SIGNAL 				= 0x08,
	TI_HEALTH_SYNC_FAIL 				= 0x10
};

enum time_output_health_flags{
	TO_HEALTH_NO_SOURCE				 	= 0x04,
	TO_HEALTH_TCG_FREEWHEEL				= 0x08
};

enum port_flags{
	PORT_1  = 0x001,
	PORT_2  = 0x002,
	PORT_3  = 0x004,
	PORT_4  = 0x008,
	PORT_5  = 0x010,
	PORT_6  = 0x020,
	PORT_7  = 0x040,
	PORT_8  = 0x080,
	PORT_9  = 0x100,
	PORT_10 = 0x200,
	PORT_11 = 0x400,
	PORT_12 = 0x800,
};

enum disc_out_flags{
	DISC_OUT_1  = 0x001,
	DISC_OUT_2  = 0x002,
	DISC_OUT_3  = 0x004,
	DISC_OUT_4  = 0x008,
	DISC_OUT_5  = 0x010,
	DISC_OUT_6  = 0x020,
	DISC_OUT_7  = 0x040,
	DISC_OUT_8  = 0x080,
	DISC_OUT_9  = 0x100,
	DISC_OUT_10 = 0x200,
	DISC_OUT_11 = 0x400,
	DISC_OUT_12 = 0x800,
	DISC_OUT_FAULT = 0x1000
};

enum led_flags{
	PORT_1_LED  = 0x001,
	PORT_2_LED  = 0x002,
	PORT_3_LED  = 0x004,
	PORT_4_LED  = 0x008,
	PORT_5_LED  = 0x010,
	PORT_6_LED  = 0x020,
	PORT_7_LED  = 0x040,
	PORT_8_LED  = 0x080,
	PORT_9_LED  = 0x100,
	PORT_10_LED = 0x200,
	PORT_11_LED = 0x400,
	PORT_12_LED = 0x800,
#if defined (BOARD_NOT_APPLICABLE_REF)
	GPS_LED_B 			= 0x10000000,//unused
	USER_1_LED_B 		= 0x20000000,//unused
	TIME_LOCK_LED_B		= 0x40000000,//time
	STATUS_LED_B 		= 0x80000000,//status
#else
	GPS_LED_B 			= 0x01000000,//gps
	TIME_LOCK_LED_B  	= 0x02000000,//time
	STATUS_LED_B 		= 0x04000000,//status
	USER_1_LED_B 		= 0x08000000,//blank
#endif
#if defined (BOARD_NOT_APPLICABLE_REF)
	GPS_LED_RG 			= 0x10000000,//unused
	USER_1_LED_RG 		= 0x20000000,//unused
	TIME_LOCK_LED_RG	= 0x40000000,//time
	STATUS_LED_RG 		= 0x80000000,//status
#else
	GPS_LED_RG 			= 0x10000000,//gps
	TIME_LOCK_LED_RG 	= 0x20000000,//time
	STATUS_LED_RG		= 0x40000000,//status
	USER_1_LED_RG 		= 0x80000000,//blank
#endif
//	USER_2_LED_B 		= 0x01000000,//gps
//	USER_1_LED_B 		= 0x02000000,//time
//	TIME_LOCK_LED_B 	= 0x04000000,//status
//	STATUS_LED_B 		= 0x08000000,//blank
//	USER_2_LED_RG 		= 0x10000000,//gps
//	USER_1_LED_RG 		= 0X20000000,//time
//	TIME_LOCK_LED_RG	= 0x40000000,//status
//	STATUS_LED_RG 		= 0x80000000,//blank
};

typedef enum {
	MITR_STATE_FAIL 	= 0,
	MITR_STATE_IDLE,
	MITR_STATE_BIT,
	MITR_STATE_BUSY,
	MITR_STATE_ERROR,
	//add new states above this line
	// be sure to also add the name to the mitr_status_name table below...
	MITR_STATE_UNKNOWN
}mitr_status_t;

static char * mitr_status_name[] = {
	[MITR_STATE_FAIL] 	= "FAIL",
	[MITR_STATE_IDLE]	= "IDLE",
	[MITR_STATE_BIT] 	= "BIT",
	[MITR_STATE_BUSY]	= "BUSY",
	[MITR_STATE_ERROR]	= "ERROR",
	// ADD NEW STATE NAMES ABOVE THIS LINE
	[MITR_STATE_UNKNOWN] = "UNKNOWN"
};


typedef enum{
	FPGA_SUBSYSTEM_NONE = 0,
	FPGA_SUBSYSTEM_DISCRETE_INPUTS 	= 1,
	FPGA_SUBSYSTEM_DISCRETE_OUTPUTS = 2,
	FPGA_SUBSYSTEM_GPS 				= 3,
	FPGA_SUBSYSTEM_TIME_INPUT 		= 4,
	FPGA_SUBSYSTEM_TIME_OUTPUT 		= 5
}mitr_fpga_subsystem_t;

typedef enum{
	// ENTER NEW SETTINGS ABOVE THIS LINE
	// DON'T FORGET TO ALSO PUT A DESCRIPTOR NAME IN THE mitr_di_setting_name table below
	DI_SETTING_DISABLED
}mitr_di_setting_t;

static char * mitr_di_setting_name[] = {
	// ADD NEW SETTING NAME ABOVE THIS LINE
	[DI_SETTING_DISABLED] = "DISABLED"
};

typedef struct discrete_in_config_t{
	mitr_di_setting_t di_setting[DISCRETE_IN_MAX];
}discrete_in_config;

typedef enum{

	DO_SETTING_P1_ACTIVITY = 0,
	DO_SETTING_P2_ACTIVITY,
	DO_SETTING_P3_ACTIVITY,
	DO_SETTING_P4_ACTIVITY,
	DO_SETTING_P5_ACTIVITY,
	DO_SETTING_P6_ACTIVITY,
	DO_SETTING_P7_ACTIVITY,
	DO_SETTING_P8_ACTIVITY,
	DO_SETTING_P9_ACTIVITY,
	DO_SETTING_P10_ACTIVITY,
	DO_SETTING_P11_ACTIVITY,	//10
	DO_SETTING_P12_ACTIVITY,
	DO_SETTING_P1_LINK,
	DO_SETTING_P2_LINK,
	DO_SETTING_P3_LINK,
	DO_SETTING_P4_LINK,
	DO_SETTING_P5_LINK,
	DO_SETTING_P6_LINK,
	DO_SETTING_P7_LINK,
	DO_SETTING_P8_LINK,
	DO_SETTING_P9_LINK,			//20
	DO_SETTING_P10_LINK,
	DO_SETTING_P11_LINK,
	DO_SETTING_P12_LINK,
	DO_SETTING_TIME_LOCK,
	DO_SETTING_1588_LOCK,
	DO_SETTING_GPS_LOCK,
	DO_SETTING_SWITCH_ERROR,
	DO_SETTING_FPGA_ERROR,
	DO_SETTING_USER,
	// ADD NEW SETTINGS ABOVE THIS LINE
	// DON'T FORGET TO ALSO PUT A DESCRIPTOR NAME IN THE mitr_do_setting_name table below
	DO_SETTING_DISABLED
}mitr_do_setting_t;

static char * mitr_do_setting_name[] = {
	[DO_SETTING_P1_ACTIVITY] 	= "PORT 1 ACTIVITY",
	[DO_SETTING_P2_ACTIVITY] 	= "PORT 2 ACTIVITY",
	[DO_SETTING_P3_ACTIVITY] 	= "PORT 3 ACTIVITY",
	[DO_SETTING_P4_ACTIVITY] 	= "PORT 4 ACTIVITY",
	[DO_SETTING_P5_ACTIVITY] 	= "PORT 5 ACTIVITY",
	[DO_SETTING_P6_ACTIVITY] 	= "PORT 6 ACTIVITY",
	[DO_SETTING_P7_ACTIVITY] 	= "PORT 7 ACTIVITY",
	[DO_SETTING_P8_ACTIVITY] 	= "PORT 8 ACTIVITY",
	[DO_SETTING_P9_ACTIVITY] 	= "PORT 9 ACTIVITY",
	[DO_SETTING_P10_ACTIVITY] 	= "PORT 10 ACTVITY",
	[DO_SETTING_P11_ACTIVITY] 	= "PORT 11 ACTIVITY",
	[DO_SETTING_P12_ACTIVITY] 	= "PORT 12 ACTIVITY",
	[DO_SETTING_P1_LINK] 		= "PORT 1 LINK",
	[DO_SETTING_P2_LINK] 		= "PORT 2 LINK",
	[DO_SETTING_P3_LINK] 		= "PORT 3 LINK",
	[DO_SETTING_P4_LINK] 		= "PORT 4 LINK",
	[DO_SETTING_P5_LINK] 		= "PORT 5 LINK",
	[DO_SETTING_P6_LINK] 		= "PORT 6 LINK",
	[DO_SETTING_P7_LINK] 		= "PORT 7 LINK",
	[DO_SETTING_P8_LINK] 		= "PORT 8 LINK",
	[DO_SETTING_P9_LINK] 		= "PORT 9 LINK",
	[DO_SETTING_P10_LINK] 		= "PORT 10 LINK",
	[DO_SETTING_P11_LINK] 		= "PORT 11 LINK",
	[DO_SETTING_P12_LINK] 		= "PORT 12 LINK",
	[DO_SETTING_TIME_LOCK] 		= "TIME LOCK",
	[DO_SETTING_1588_LOCK] 		= "IEEE-1588 LOCK",
	[DO_SETTING_GPS_LOCK] 		= "GPS LOCK",
	[DO_SETTING_SWITCH_ERROR] 	= "SWITCH ERROR",
	[DO_SETTING_FPGA_ERROR] 	= "FPGA ERROR",
	[DO_SETTING_USER] 			= "USER CONTROL",
	// ADD NEW SETTING NAME ABOVE THIS LINE
	[DO_SETTING_DISABLED] 		= "DISABLED"
};

typedef struct discrete_out_config_t{
	mitr_do_setting_t do_setting[DISCRETE_OUT_MAX];
}discrete_out_config;

/*
 * time input types
 */

typedef enum {
	DC_BIAS_0 = 0,
	DC_BIAS_3_3,
	DC_BIAS_5
}mitr_gps_dc_bias_t;

static char * GPS_DC_BIAS_STR[] = {
	[DC_BIAS_0] = "0 Volts",
	[DC_BIAS_3_3] = "3.3 Volts",
	[DC_BIAS_5] = "5 Volts"
};

typedef enum {
	TIME_INPUT_SRC_IRIG_B = 0,
	TIME_INPUT_SRC_IRIG_A,
	TIME_INPUT_SRC_IRIG_G,
	TIME_INPUT_SRC_1588,
	TIME_INPUT_SRC_GPS0,
	TIME_INPUT_SRC_GPS3,
	TIME_INPUT_SRC_GPS5,
	TIME_INPUT_SRC_DISABLED
}mitr_time_input_src_t;

#define TIME_IN_CAL_TABLE_ENTRIES 5

static char * TIME_IN_SRC_STR[] = {
	[TIME_INPUT_SRC_IRIG_B] = "IRIG-B",
	[TIME_INPUT_SRC_IRIG_A] = "IRIG-A",
	[TIME_INPUT_SRC_IRIG_G] = "IRIG-G",
	[TIME_INPUT_SRC_1588] = "IEEE-1588",
	[TIME_INPUT_SRC_GPS0] = "GPS",
	[TIME_INPUT_SRC_GPS3] = "GPS",
	[TIME_INPUT_SRC_GPS5] = "GPS",
	[TIME_INPUT_SRC_DISABLED] = "DISABLED"
};

typedef enum {
	TIME_INPUT_TYPE_DC = 0,
	TIME_INPUT_TYPE_RESERVED,
	TIME_INPUT_TYPE_LOOPBACK,
	TIME_INPUT_TYPE_AM
}mitr_time_input_signal_t;

static char * TIME_IN_SIGNAL_STR[] = {
	[TIME_INPUT_TYPE_DC] = "DC",
	[TIME_INPUT_TYPE_RESERVED] = "RESERVED",
	[TIME_INPUT_TYPE_LOOPBACK] = "LOOPBACK",
	[TIME_INPUT_TYPE_AM] = "AM"
};

typedef struct mitr_time_input_t{
	u32 channel_id;
	mitr_time_input_src_t source;
	mitr_time_input_signal_t signal;
}mitr_time_input;

typedef struct mitr_time_in_cal_t{
	u16 cal;
	u16 offset_multiplier;
}mitr_time_in_cal;

/*
 * time output types
 */

typedef enum{
	TIME_OUTPUT_TYPE_IRIG_B = 0,
	TIME_OUTPUT_TYPE_IRIG_A,
	TIME_OUTPUT_TYPE_IRIG_G,
	TIME_OUTPUT_TYPE_DISABLED
}mitr_time_output_t;

#define TIME_OUT_CAL_TABLE_ENTRIES	3

static char * TIME_OUT_TYPE_STR[] = {
	[TIME_OUTPUT_TYPE_IRIG_B] = "IRIG-B",
	[TIME_OUTPUT_TYPE_IRIG_A] = "IRIG-A",
	[TIME_OUTPUT_TYPE_IRIG_G] = "IRIG-G",
	[TIME_OUTPUT_TYPE_DISABLED] = "DISABLED"
};

typedef enum{
	IEEE_1588_DISABLED = 0,
	RESERVED1,
	IEEE_1588_SLAVE,
	IEEE_1588_MASTER
}mitr_1588_type;

static char * IEEE_1588_TYPE_STR[] = {
	[IEEE_1588_DISABLED] = "DISABLED",
	[RESERVED1] = "RESERVED",
	[IEEE_1588_SLAVE] = "IEEE-1588 SLAVE",
	[IEEE_1588_MASTER] = "IEEE-1588 MASTER"
};

typedef enum{
	TIME_OUTPUT_MODE_RESERVED0 = 0,
	TIME_OUTPUT_MODE_TCG,
	TIME_OUTPUT_MODE_RESERVED2,
	TIME_OUTPUT_MODE_TEST
}mitr_time_output_mode_t;

static char * TIME_OUTPUT_MODE_STR[] = {
	[TIME_OUTPUT_MODE_RESERVED0] = "RESERVED",
	[TIME_OUTPUT_MODE_TCG] = "TCG",
	[TIME_OUTPUT_MODE_RESERVED2] = "RESERVED",
	[TIME_OUTPUT_MODE_TEST] = "TEST-DEBUG"
};

typedef struct mitr_time_output_t{
	u32 channel_id;
	mitr_time_output_t timecode;
	mitr_1588_type ieee_1588_type;
	mitr_time_output_mode_t mode;
}mitr_time_output;


typedef struct mitr_time_out_cal_t{
	u16 coarse_cal;
	u16 med_cal;
	u16 fine_cal;
}mitr_time_out_cal;

typedef struct mitr_bcd_time_t{
	int bcd_s; 	// seconds
	int bcd_ts; 	// tens of seconds
	int bcd_m; 	// minutes
	int bcd_tm; 	// tens of minutes
	int bcd_h; 	// hours
	int bcd_th;	// tens of hours
	int bcd_d;	// days
	int bcd_td;	// tens of days
	int bcd_hd;	// hundreds of days
	int bcd_y;	// years (since 2000)
	int bcd_ty;	// tens of years (since 2000)
}mitr_bcd_time;

typedef struct mitr_fpga_conf{
	int version;
	BOOL verbose;
	discrete_in_config di_config;
	discrete_out_config do_config;
	mitr_time_input time_in_setting;
	mitr_time_output time_out_setting;
	mitr_time_in_cal time_in_calibration[TIME_IN_CAL_TABLE_ENTRIES];
	mitr_time_out_cal time_out_calibration[TIME_OUT_CAL_TABLE_ENTRIES];
}mitr_fpga_conf_t;

#endif /* __MITR_I2C_TYPES_H__*/
