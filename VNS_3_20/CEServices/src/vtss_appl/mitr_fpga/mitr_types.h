#ifndef __MITR_I2C_TYPES_H__
#define __MITR_I2C_TYPES_H__

//
//#if defined (BOARD_MITR_REF)
//#define DISCRETE_IN_MAX		4
//#define DISCRETE_OUT_MAX	12
//#define DISCRETE_OUT_MASK	0xFFF
//#define MITR_PORT_COUNT		12
//#define MODEL_NAME "MITR_SWITCH"
//#elif defined (BOARD_NOT_APPLICABLE_REF)
//#define DISCRETE_IN_MAX		1
//#define DISCRETE_OUT_MAX	8
//#define DISCRETE_OUT_MASK	0xFF
//#define MITR_PORT_COUNT		6
//#endif
//
//#define MITR_CONFIG_VERSION 	5
#define MITR_VERSION_MAJOR	2
#define MITR_VERSION_MINOR	1
//
//#define MANUFACTURER_NAME "Telspan Data"
//
typedef enum{
	MITR_ERROR_INVALID_COMMAND = 0,
	MITR_ERROR_INVALID_PARAMETER = 1,
	MITR_ERROR_INVALID_MODE = 2,
	MITR_ERROR_NO_MEDIA = 3,
	MITR_ERROR_MEDIA_FULL = 4,
	MITR_ERROR_COMMAND_FAILED = 5
}mitr_fpga_error_code_t;
//
//typedef enum{
//	MITR_API_CMD_SUCCESS = 0,
//	MITR_API_CMD_BUSY,
//	MITR_API_CMD_FAIL
//}mitr_api_cmd_err;
//
extern const char * MITR_ERROR_CODE_STR[];
//
//typedef enum{
//	DIRECTION_NONE = 0,
//	DIRECTION_IN,
//	DIRECTION_OUT
//}mitr_direction_t;
//
//enum common_health_flags{
//	HEALTH_BIT_FAIL 	= 0x01,
//	HEALTH_SETUP_FAIL 	= 0x02
//};
//
//enum system_health_flags{
//	SYS_HEALTH_OPERATION_FAIL 			= 0x04,
//	SYS_HEALTH_BUSY 					= 0x08,
//	SYS_HEALTH_POWER_BOARD_OVER_TEMP 	= 0x10,
//	SYS_HEALTH_FPGA_BOARD_PSC_FAIL 		= 0x80,
//	SYS_HEALTH_GPS_FAULT 				= 0x100
//};
//
//typedef enum {
//	MITR_TIME_INPUT_SRC_IRIG_B = 0,
//	MITR_TIME_INPUT_SRC_IRIG_A,
//	MITR_TIME_INPUT_SRC_IRIG_G,
//	MITR_TIME_INPUT_SRC_1588,
//	MITR_TIME_INPUT_SRC_GPS,
//	MITR_TIME_INPUT_SRC_MIL_STD_1553,
//	MITR_TIME_INPUT_SRC_RESERVED6,
//	MITR_TIME_INPUT_SRC_DISABLED
//}mitr_time_input_src_t;
//
//enum led_flags{
//	PORT_1_LED  = 0x001,
//	PORT_2_LED  = 0x002,
//	PORT_3_LED  = 0x004,
//	PORT_4_LED  = 0x008,
//	PORT_5_LED  = 0x010,
//	PORT_6_LED  = 0x020,
//	PORT_7_LED  = 0x040,
//	PORT_8_LED  = 0x080,
//	PORT_9_LED  = 0x100,
//	PORT_10_LED = 0x200,
//	PORT_11_LED = 0x400,
//	PORT_12_LED = 0x800,
////#if defined (BOARD_NOT_APPLICABLE_REF)
////	GPS_LED_B 			= 0x10000000,//unused
////	USER_1_LED_B 		= 0x20000000,//unused
////	TIME_LOCK_LED_B		= 0x40000000,//time
////	STATUS_LED_B 		= 0x80000000,//status
////#else
////	GPS_LED_B 			= 0x01000000,//gps
////	TIME_LOCK_LED_B  	= 0x02000000,//time
////	STATUS_LED_B 		= 0x04000000,//status
////	USER_1_LED_B 		= 0x08000000,//blank
////#endif
////#if defined (BOARD_NOT_APPLICABLE_REF)
////	GPS_LED_RG 			= 0x10000000,//unused
////	USER_1_LED_RG 		= 0x20000000,//unused
////	TIME_LOCK_LED_RG	= 0x40000000,//time
////	STATUS_LED_RG 		= 0x80000000,//status
////#else
////	GPS_LED_RG 			= 0x10000000,//gps
////	TIME_LOCK_LED_RG 	= 0x20000000,//time
////	STATUS_LED_RG		= 0x40000000,//status
////	USER_1_LED_RG 		= 0x80000000,//blank
////#endif
//////	USER_2_LED_B 		= 0x01000000,//gps
//////	USER_1_LED_B 		= 0x02000000,//time
//////	TIME_LOCK_LED_B 	= 0x04000000,//status
//////	STATUS_LED_B 		= 0x08000000,//blank
//////	USER_2_LED_RG 		= 0x10000000,//gps
//////	USER_1_LED_RG 		= 0X20000000,//time
//////	TIME_LOCK_LED_RG	= 0x40000000,//status
//////	STATUS_LED_RG 		= 0x80000000,//blank
//};
//
//enum disc_out_flags{
//	DISC_OUT_FAULT  = 0x001,
//	DISC_OUT_BIT  = 0x002,
//	DISC_OUT_DECLASS  = 0x004,
//	DISC_OUT_RECORD  = 0x008,
//	DISC_OUT_ERASE  = 0x010,
//	DISC_OUT_TVAL  = 0x020,
//	DISC_OUT_DATA_ERR1  = 0x040,
//	DISC_OUT_DATA_ERR2  = 0x080,
//	DISC_OUT_DATA_ERR3  = 0x100,
//	DISC_OUT_PORT1 = 0x200,
//	DISC_OUT_PORT2 = 0x400,
//	DISC_OUT_PORT3 = 0x800,
//	DISC_OUT_PORT4 = 0x1000
//};
//
//typedef enum {
//	MITR_STATE_FAIL 	= 0,
//	MITR_STATE_IDLE,
//	MITR_STATE_BIT,
//	MITR_STATE_BUSY,
//	MITR_STATE_ERROR,
//	//add new states above this line
//	// be sure to also add the name to the mitr_status_name table below...
//	MITR_STATE_UNKNOWN
//}mitr_status_t;
//
//extern const char * mitr_status_name[];
//
//
////typedef enum{
////	FPGA_SUBSYSTEM_NONE = 0,
////#if !defined(BOARD_IGU_REF)
////	FPGA_SUBSYSTEM_DISCRETE_INPUTS,
////	FPGA_SUBSYSTEM_DISCRETE_OUTPUTS,
////#if defined(BOARD_MITR_REF)
////	FPGA_SUBSYSTEM_GPS,
////#endif //defined(BOARD_MITR_REF)
////#endif //!defined(BOARD_IGU_REF)
////	FPGA_SUBSYSTEM_TIME_INPUT,
////#if !defined(BOARD_IGU_REF)
////	FPGA_SUBSYSTEM_TIME_OUTPUT,
////#endif //!defined(BOARD_IGU_REF)
////	FPGA_SUBSYSTEM_NA
////}mitr_fpga_subsystem_t;
//
//
//
//typedef enum{
//	MITR_IEEE_1588_DISABLED = 0,
//	MITR_IEEE_1588_RESERVED1,
//	MITR_IEEE_1588_SLAVE,
//	MITR_IEEE_1588_MASTER
//}mitr_1588_type;
//
//extern const char * IEEE_1588_TYPE_STR[];
//
//
//
//typedef struct mitr_bcd_time_t{
//	int bcd_s; 	// seconds
//	int bcd_ts; 	// tens of seconds
//	int bcd_m; 	// minutes
//	int bcd_tm; 	// tens of minutes
//	int bcd_h; 	// hours
//	int bcd_th;	// tens of hours
//	int bcd_d;	// days
//	int bcd_td;	// tens of days
//	int bcd_hd;	// hundreds of days
//	int bcd_y;	// years (since 2000)
//	int bcd_ty;	// tens of years (since 2000)
//}mitr_bcd_time;
//
//typedef struct mitr_fpga_conf{
//	int version;
//	BOOL verbose;
//	mitr_1588_type ieee_1588_mode;
//}mitr_fpga_conf_t;

#endif /* __MITR_I2C_TYPES_H__*/
