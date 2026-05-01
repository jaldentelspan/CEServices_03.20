#ifndef __VNS_I2C_TYPES_H__
#define __VNS_I2C_TYPES_H__
#include "vtss_api.h"

#ifdef SHOW_IRIG_INPUTS
#define HIDE_IRIG_INPUTS	0
#else
#define HIDE_IRIG_INPUTS	1
#endif

/* #if EPE_FPGA */
/* #define IES_EPE_FEATURE_ENABLE */
/* #else /1* EPE_FPGA *1/ */

/* #endif /1* EPE_FPGA *1/ */
#define VNS_EPE_BIT_RATE_MIN    (2000)
#define VNS_EPE_BIT_RATE_MAX    (20000)
#define VNS_EPE_PERCENT_RATE_MIN    (( VNS_EPE_BIT_RATE_MIN * 100)/VNS_EPE_BIT_RATE_MAX)
#define VNS_EPE_PERCENT_RATE_MAX    (( VNS_EPE_BIT_RATE_MAX * 100)/VNS_EPE_BIT_RATE_MAX)
#define VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MIN  (1)
#define VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MAX  (8)
#define VNS_EPE_ENABLE_BIT      (0x0001)
#define FPGA_EPE_HEADER_SIGNATURE	0x4550454C 

#if defined (BOARD_VNS_12_REF) 
#define DISCRETE_IN_MAX		4
#define DISCRETE_OUT_MAX	12
#define DISCRETE_OUT_MASK	0xFFF
#define VNS_PORT_COUNT		12
//#define HIDE_IRIG_INPUTS	1
#define MODEL_NAME "iES-12"
#define VNS_MAC_PRODUCT_ID (0x11)
#elif defined (BOARD_VNS_8_REF)
#define VNS_MAC_PRODUCT_ID (0x1D)
#define DISCRETE_IN_MAX		1 
#define DISCRETE_OUT_MAX	0 
#define DISCRETE_OUT_MASK	0x00
#define VNS_PORT_COUNT		8
//#define HIDE_IRIG_INPUTS	0
#define MODEL_NAME "iES-8"
/* #define FPGA_UPDATE_HEADER_SIGNATURE	0x69455306 */
#elif defined (BOARD_VNS_6_REF)
#define VNS_MAC_PRODUCT_ID (0x12)
#define DISCRETE_IN_MAX		1 
#define DISCRETE_OUT_MAX	8 
#define DISCRETE_OUT_MASK	0xFF
#define VNS_PORT_COUNT		6
//#define HIDE_IRIG_INPUTS	0
#define MODEL_NAME "iES-6"
/* #define FPGA_UPDATE_HEADER_SIGNATURE	0x69455306 */
#elif defined (BOARD_NCGU_REF)
#define VNS_MAC_PRODUCT_ID (0x17)
#define DISCRETE_IN_MAX		4
#define DISCRETE_OUT_MAX	4
#define DISCRETE_OUT_MASK	0xF
#define VNS_PORT_COUNT		6
//#define HIDE_IRIG_INPUTS	1
#define MODEL_NAME "NCGU"
#elif defined (BOARD_IGU_REF)
#define VNS_MAC_PRODUCT_ID (0x20)
#define DISCRETE_IN_MAX		4
#define DISCRETE_OUT_MAX	4
#define DISCRETE_OUT_MASK	0xF
#define VNS_PORT_COUNT		6
//#define HIDE_IRIG_INPUTS	1
#define MODEL_NAME "IGU"
#else
#define MODEL_NAME "????"
#error "Please add Board Definition! "
#endif

#define VNS_EPE_PORT            ( VNS_PORT_COUNT +2  )

#ifndef DISCRETE_IN_MAX
#error VNS_FPGA DISCRETE_IN_MAX NOT SET...
#endif

#ifndef HIDE_IRIG_INPUTS
#define HIDE_IRIG_INPUTS	1
#endif

#define VNS_CONFIG_VERSION 	9
#define VNS_VERSION_MAJOR	1
#define VNS_VERSION_MINOR	19

#define MANUFACTURER_NAME "Telspan Data"

typedef enum {
    /* iES-6 */
    VNS_FPGA_UPDATE_HEADER_SIG_IES06_V1 = 0x69455306,
    /* iES-12 with FPGA board REV 1-3 */
    VNS_FPGA_UPDATE_HEADER_SIG_IES12_V1= 0x69455312,
    /* iES-12 with FPGA board REV 4 */
    VNS_FPGA_UPDATE_HEADER_SIG_IES12_V2 = 0x69455322
} vns_fpga_update_header_signature_t;

/* debug ies epe lic 0x9D7F4C93 0x70CB34DC 0x4550454C 0x69455322 0xE8110000 */
typedef enum {
     ies_fpga_epe_global_license_0 = 0x9D7F4C93,
     ies_fpga_epe_global_license_1 = 0x70CB34DC,
     ies_fpga_epe_global_license_2 = 0x4550454C,
     ies_fpga_epe_global_license_3 = 0x69455322,
     ies_fpga_epe_global_license_4 = 0xE8110000
} ies_fpga_epe_global_license_t;

typedef enum{
	VNS_ERROR_INVALID_COMMAND   = 0,
	VNS_ERROR_INVALID_PARAMETER = 1,
	VNS_ERROR_INVALID_MODE      = 2,
	VNS_ERROR_NO_MEDIA          = 3,
	VNS_ERROR_MEDIA_FULL        = 4,
	VNS_ERROR_COMMAND_FAILED    = 5
}vns_fpga_error_code_t;

typedef enum{
	VNS_API_CMD_SUCCESS = 0,
	VNS_API_CMD_BUSY,
	VNS_API_CMD_FAIL
}vns_api_cmd_err;

extern const char * VNS_ERROR_CODE_STR[];

typedef enum{
	DIRECTION_NONE = 0,
	DIRECTION_IN,
	DIRECTION_OUT
}vns_direction_t;

enum common_health_flags{
	HEALTH_BIT_FAIL 	= 0x01,
	HEALTH_SETUP_FAIL 	= 0x02
};

enum system_health_flags{
	SYS_HEALTH_OPERATION_FAIL        = 0x04,
	SYS_HEALTH_BUSY                  = 0x08,
	SYS_HEALTH_POWER_BOARD_OVER_TEMP = 0x10,
	SYS_HEALTH_FPGA_BOARD_PSC_FAIL   = 0x80,
	SYS_HEALTH_GPS_FAULT             = 0x100
};

enum gps_health_flags{
	GPS_HEALTH_NO_ANTENNA_SIGNAL  = 0x04,
	GPS_HEALTH_BAD_ANTENNA_SIGNAL = 0x08,
	GPS_HEALTH_TIME_SYNC_FAIL     = 0x10,
	GPS_HEALTH_NO_SATELLITES      = 0x20
};

enum time_input_health_flags{
	TI_HEALTH_NO_SIGNAL  = 0x04,
	TI_HEALTH_BAD_SIGNAL = 0x08,
	TI_HEALTH_SYNC_FAIL  = 0x10
};

enum time_output_health_flags{
	TO_HEALTH_NO_SOURCE     = 0x04,
	TO_HEALTH_TCG_FREEWHEEL = 0x08
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
	DISC_OUT_1     = 0x001,
	DISC_OUT_2     = 0x002,
	DISC_OUT_3     = 0x004,
	DISC_OUT_4     = 0x008,
	DISC_OUT_5     = 0x010,
	DISC_OUT_6     = 0x020,
	DISC_OUT_7     = 0x040,
	DISC_OUT_8     = 0x080,
	DISC_OUT_9     = 0x100,
	DISC_OUT_10    = 0x200,
	DISC_OUT_11    = 0x400,
	DISC_OUT_12    = 0x800,
	DISC_OUT_FAULT = 0x1000
};

enum led_flags{
	PORT_1_LED       = 0x001,
	PORT_2_LED       = 0x002,
	PORT_3_LED       = 0x004,
	PORT_4_LED       = 0x008,
	PORT_5_LED       = 0x010,
	PORT_6_LED       = 0x020,
	PORT_7_LED       = 0x040,
	PORT_8_LED       = 0x080,
	PORT_9_LED       = 0x100,
	PORT_10_LED      = 0x200,
	PORT_11_LED      = 0x400,
	PORT_12_LED      = 0x800,
#if defined (BOARD_VNS_6_REF) || defined (BOARD_VNS_8_REF)
	// GPS_LED_B        = 0x10000000,//unused
	// USER_1_LED_B     = 0x20000000,//unused
	// TIME_LOCK_LED_B  = 0x40000000,//time
	// STATUS_LED_B     = 0x80000000,//status
	GPS_LED_B        = 0x01000000,//gps
	USER_1_LED_B     = 0x02000000,//blank
	TIME_LOCK_LED_B  = 0x04000000,//time
	STATUS_LED_B     = 0x08000000,//status
#else
	GPS_LED_B        = 0x01000000,//gps
	TIME_LOCK_LED_B  = 0x02000000,//time
	STATUS_LED_B     = 0x04000000,//status
	USER_1_LED_B     = 0x08000000,//blank
#endif
#if defined (BOARD_VNS_6_REF) ||  defined (BOARD_VNS_8_REF)
	GPS_LED_RG       = 0x10000000,//unused
	USER_1_LED_RG    = 0x20000000,//unused
	TIME_LOCK_LED_RG = 0x40000000,//time
	STATUS_LED_RG    = 0x80000000,//status
#else
	GPS_LED_RG       = 0x10000000,//gps
	TIME_LOCK_LED_RG = 0x20000000,//time
	STATUS_LED_RG    = 0x40000000,//status
	USER_1_LED_RG    = 0x80000000,//blank
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
	VNS_STATE_FAIL 	= 0,
	VNS_STATE_IDLE,
	VNS_STATE_BIT,
	VNS_STATE_BUSY,
	VNS_STATE_ERROR,
	//add new states above this line
	// be sure to also add the name to the vns_status_name table below...
	VNS_STATE_UNKNOWN
}vns_status_t;

extern const char * vns_status_name[];


typedef enum{
	FPGA_SUBSYSTEM_NONE = 0,
#if !defined(BOARD_IGU_REF)
	FPGA_SUBSYSTEM_DISCRETE_INPUTS,
	FPGA_SUBSYSTEM_DISCRETE_OUTPUTS,
#if defined(BOARD_VNS_12_REF)
	FPGA_SUBSYSTEM_GPS,
#endif //defined(BOARD_VNS_12_REF)
#endif //!defined(BOARD_IGU_REF)
	FPGA_SUBSYSTEM_TIME_INPUT,
#if !defined(BOARD_IGU_REF)
	FPGA_SUBSYSTEM_TIME_OUTPUT,
#endif //!defined(BOARD_IGU_REF)
	FPGA_SUBSYSTEM_NA
}vns_fpga_subsystem_t;

typedef enum{
	// ENTER NEW SETTINGS ABOVE THIS LINE
	// DON'T FORGET TO ALSO PUT A DESCRIPTOR NAME IN THE vns_di_setting_name table below
	DI_SETTING_DISABLED
}vns_di_setting_t;

extern const char * vns_di_setting_name[];

typedef struct discrete_in_config_t{
	vns_di_setting_t di_setting[DISCRETE_IN_MAX];
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
	// DON'T FORGET TO ALSO PUT A DESCRIPTOR NAME IN THE vns_do_setting_name table below
	DO_SETTING_DISABLED
}vns_do_setting_t;

extern const char * vns_do_setting_name[];

typedef struct discrete_out_config_t{
	vns_do_setting_t do_setting[DISCRETE_OUT_MAX];
}discrete_out_config;

/*
 * time input types
 */

typedef enum {
	DC_BIAS_0 = 0,
	DC_BIAS_3_3,
	DC_BIAS_5
}vns_gps_dc_bias_t;

extern const char * GPS_DC_BIAS_STR[];

typedef enum {
	TIME_INPUT_SRC_IRIG_B = 0,
	TIME_INPUT_SRC_IRIG_A,
	TIME_INPUT_SRC_IRIG_G,
	TIME_INPUT_SRC_1588,
	TIME_INPUT_SRC_GPS0,
	TIME_INPUT_SRC_GPS3,
	TIME_INPUT_SRC_GPS5,
	TIME_INPUT_SRC_INTERNAL,
	TIME_INPUT_SRC_1PPS,


        /* keep new entries above */
	TIME_INPUT_SRC_DISABLED
}vns_time_input_src_t;

#define TIME_IN_CAL_TABLE_ENTRIES 9

extern const char * TIME_IN_SRC_ENUM_STR[];
extern const char * TIME_IN_SRC_STR[];
extern const char * TIME_IN_SRC_VOLTAGE_STR[];

typedef enum {
	TIME_INPUT_TYPE_DC = 0,
	TIME_INPUT_TYPE_RESERVED,
	TIME_INPUT_TYPE_LOOPBACK,
	TIME_INPUT_TYPE_AM
}vns_time_input_signal_t;

extern const char * TIME_IN_SIGNAL_STR[];

typedef struct vns_time_input_t{
	u32 channel_id;
	vns_time_input_src_t source;
	vns_time_input_signal_t signal;
}vns_time_input;

typedef struct vns_time_in_cal_t{
	u16 cal;
	u16 offset_multiplier;
}vns_time_in_cal;

/*
 * time output types
 */

typedef enum{
	TIME_OUTPUT_TYPE_IRIG_B = 0,
	TIME_OUTPUT_TYPE_IRIG_A,
	TIME_OUTPUT_TYPE_IRIG_G,
	TIME_OUTPUT_TYPE_DISABLED
}vns_time_output_t;

#define TIME_OUT_CAL_TABLE_ENTRIES	3

extern const char * TIME_OUT_TYPE_STR[];

typedef enum{
	IEEE_1588_DISABLED = 0,
	RESERVED1,
	IEEE_1588_SLAVE,
	IEEE_1588_MASTER
}vns_1588_type;

extern const char * IEEE_1588_TYPE_STR[];

typedef enum{
	TIME_OUTPUT_MODE_RESERVED0 = 0,
	TIME_OUTPUT_MODE_TCG,
	TIME_OUTPUT_MODE_RESERVED2,
	TIME_OUTPUT_MODE_TEST
}vns_time_output_mode_t;

extern const char * TIME_OUTPUT_MODE_STR[];

typedef struct vns_time_output_t{
	u32 channel_id;
	vns_time_output_t timecode;
	vns_1588_type ieee_1588_type;
	vns_time_output_mode_t mode;
	BOOL irig_dc_1pps_mode;
	u8 reserved1;  // align to 32 bits
        // making these bits usefull by assighning them to hold leap_seconds
	/* u16 reserved2; // align to 32 bits */
	u16 leap_seconds; // align to 32 bits
}vns_time_output;


typedef struct vns_time_out_cal_t{
	u16 coarse_cal;
	u16 med_cal;
	u16 fine_cal;
	u16 reserved1; //align to 32 bits
}vns_time_out_cal;

typedef struct vns_bcd_time_t{
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
}vns_bcd_time;

/* EPE Configuration Details */
extern const char * VNS_EPE_MODE_STR[];

typedef enum {
    VNS_EPE_DISABLE       = 0x00,
    VNS_EPE_HDLC          = 0x01,
    VNS_EPE_CH7_15        = 0x02,
    VNS_EPE_DECODE_HDLC   = 0x03,
    VNS_EPE_DECODE_CH7_15 = 0x04,
    VNS_EPE_END
} vns_epe_mode_t;

typedef struct {
    u32 reserved1;
    u32 reserved2;
    u32 ies_signature;
    u32 epe_signature;
    u32 key;
}vns_fpga_epe_license_info_t;

typedef struct {
    vns_epe_mode_t mode;
    u32 bit_rate;
    u8 ch7_data_word_count;
    u8 reserved1;  /*for 32 bit alignment */
    u16 reserved2; /*for 32 bit alignment */
} vns_epe_conf_blk_t;

typedef enum {
    FPGA_UPDATE_INVALID_N0 = 0x00,
    FPGA_UPDATE_INVALID_N1,
    FPGA_UPDATE_INVALID_N2,
    FPGA_UPDATE_INVALID_N3,
    FPGA_UPDATE_INVALID_N4,
    FPGA_UPDATE_INVALID_N5,
    FPGA_UPDATE_INVALID_N6,
    FPGA_UPDATE_INVALID_N7,
    FPGA_UPDATE_INVALID_N8,
    FPGA_UPDATE_INVALID_N9
} fpga_update_invalid_type_t;

typedef enum {
    IES_BOARDID_POWER_BRD_REV,     
    IES_BOARDID_FPGA_BRD_REV,      
    IES_BOARDID_SWITCH_BRD_REV,    
    IES_BOARDID_INTERCONNECT_BRD_REV,
    IES_BOARDID_END,
} ies_board_id_t;

    /* if bit 0 is zero, the standard ies will be prevelent. At this time irig input is avalible if bit 0 is= 0 */
    /* if bit 0 equal to zero irig input will be enabled */
typedef enum {
    IES_FEATURE_EPE     = 0x01,
    IES_FEATURE_IRIG_IN = 0x02,
    IES_FEATURE_DUMMY_2 = 0x04,
    IES_FEATURE_DUMMY_3 = 0x08
} ies_feature_set_t;

typedef struct {
    u32 mask;
    u8 shift;
    char* name;
}ies_board_id_info_t;

typedef struct {
    u8 revision;
    char* name;
}ies_board_rev_info_t;

typedef struct vns_fpga_conf{
    int version;
    BOOL verbose;
    u8 reserved1;  // align to 32 bits
    u16 reserved2; // align to 32 bits
    vns_time_input time_in_setting;
    vns_time_output time_out_setting;
    vns_time_in_cal time_in_calibration[TIME_IN_CAL_TABLE_ENTRIES];
    vns_time_out_cal time_out_calibration[TIME_OUT_CAL_TABLE_ENTRIES];
    discrete_in_config di_config;
    discrete_out_config do_config;
    vns_epe_conf_blk_t epe_config;
}vns_fpga_conf_t;

typedef enum {
    IES_TIME_PROCESS_COMMAND_SET     = 0X00,
    IES_TIME_PROCESS_COMMAND_ADD,
    IES_TIME_PROCESS_COMMAND_SUBTRACT,
    IES_TIME_PROCESS_COMMAND_END
} ies_time_process_command_t;

typedef struct vns_time {
    ies_time_process_command_t command;
    int year;
    int yday;
    int hour;
    int minute;
    int second;
} vns_time_t;


#endif /* __VNS_I2C_TYPES_H__*/
