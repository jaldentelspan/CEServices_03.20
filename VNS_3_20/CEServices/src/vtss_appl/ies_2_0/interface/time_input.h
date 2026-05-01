/*
 * time_input.h
 *
 *  Created on: Oct 22, 2019
 *      Author: eric
 */

#ifndef TIME_INPUT_H_
#define TIME_INPUT_H_

/* #include <stdint.h> */
/* #include <stdbool.h> */
#include "main.h"
#include "tsd_manual_inc_time.h"
#include "tsd_rx_irig_core.h"
#include "tsd_rx_1pps_core.h"
#include "tsd_gps_core.h"
#include "tsd_time_input_selector.h"
#include "tsd_rtc_calibrator.h"

#define UNUSED_BASE_ADDRESS	0xFFFFFFFF

/*
 * defines the entire time input subsystem
 * if any of the modules are not available in the system set their
 * base address to UNUSED_BASE_ADDRESS (0xFFFFFFFF) so that attempts to
 * read and write to the (unavailable) component are not tried.
 */
typedef struct sTimeInputSystemAddresses{
	uint32_t timeRTCCalBaseAddr;  /* RTC Calibrator (contains the PID controller to set how fast things are running) */
	uint32_t manualIncrementTimeBaseAddr; /* manual incrementer time input (time source when nothing is locked) */
	uint32_t gpsBaseAddr;         /* GPS Time input */
	uint32_t irigAMCoreBaseAddr;  /* IRIG AM Time input */
	uint32_t irigDCCoreBaseAddr;  /* IRIG DC Time input */
	uint32_t ptp1PPSCoreBaseAddr; /* 1588 Slave 1PPS Time input */
	uint32_t external1PPSCoreBaseAddr; /* external 1PPS Time input */
	uint32_t timeSelectorBaseAddr;/* time input selector */
}sTimeInputSystemAddresses_t;

typedef enum eGPSBaudRate{
	GPS_BAUDRATE_4800 = 4800,
	GPS_BAUDRATE_9600 = 9600,
	GPS_BAUDRATE_19200 = 19200,
	GPS_BAUDRATE_38400 = 38400,
	GPS_BAUDRATE_57600 = 57600,
	GPS_BAUDRATE_115200 = 115200
}eGPSBaudRate_t;

typedef enum eGPSAntennaVoltage{
	GPS_ANTENNA_VOLTAGE_0V = 0,
	GPS_ANTENNA_VOLTAGE_3P3V,
	GPS_ANTENNA_VOLTAGE_5P0V
}eGPSAntennaVoltage_t;

typedef enum {
	TIMECODEHEALTH_BIT_FAILURE 		= 0x01,
	TIMECODEHEALTH_SETUP_FAILURE		= 0x02,
	TIMECODEHEALTH_NO_EXTERNAL_SIGNAL	= 0x04,
	TIMECODEHEALTH_BAD_EXTERNAL_SIGNAL	= 0x08,
	TIMECODEHEALTH_SYNCHRONIZE_FAILURE	= 0x10,
	TIMECODEHEALTH_RTC_NOT_SYNCED		= 0x20, /* REAL TIME CLOCK */
	TIMECODEHEALTH_UNKNOWN				= 0x40
}timeCodeHealthBits_t;

typedef enum {
	TSD_TIME_INPUT_SRC_IRIG_AM			= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_0,
	TSD_TIME_INPUT_SRC_IRIG_DC			= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_1,
	TSD_TIME_INPUT_SRC_GPS				= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_2,
	TSD_TIME_INPUT_SRC_1588				= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_3,
	TSD_TIME_INPUT_SRC_EXTERNAL_1PPS	= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_4,
	TSD_TIME_INPUT_SRC_RSVD6			= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_5,
	TSD_TIME_INPUT_SRC_RSVD7			= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_6,
	TSD_TIME_INPUT_SRC_INTERNAL			= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_7
}eTimeInputSource_t;

extern const char *eTimeInputSourceString[];

typedef struct sTimeInputSystem
{
	sTimeInputSystemAddresses_t addrInfo;
	uint32_t qysModulesClkFrequency;

	/*
	 * Manual Incrementer settings Settings
	 */
	uint32_t man_inc_reset_offset; /* maximum number of RTC ticks that we can be off before abruptly resetting the output time */

	/*
	 * RTC Calibrator Settings
	 */
	bool rtc_cal_pid_enabled;
	bool rtc_cal_100mhz_cal_enabled;

	/*
	 * RX IRIG DC settings
	 */
	bool irig_dc_enabled;
	tsd_rx_irig_code_type_t irig_dc_code_type;

	/*
	 * RX IRIG AM settings
	 */
	bool irig_am_enabled;
	tsd_rx_irig_code_type_t irig_am_code_type;

	/*
	 * GPS settings
	 */
	bool gps_enabled;
	eGPSBaudRate_t gps_baudrate;
	eGPSAntennaVoltage_t gps_antenna_voltage;
	int gps_leap_second_val;

	/*
	 * 1588 Slave PPS Time Input Settings
	 */
	bool ieee_1588_slave_enabled;
	// none at this time

	/*
	 * External 1PPS Time Input Settings
	 */
	bool external_1PPS_slave_enabled;
	// none at this time

	/*
	 * Time Selector Settings
	 */
	uint8_t irig_dc_prio;
	uint8_t	irig_am_prio;
	uint8_t gps_prio;
	uint8_t ieee_1588_prio;
	uint8_t external_1pps_prio;


}sTimeInputSystem_t;

void time_input_debug_print(sTimeInputSystem_t *pTimeInSystem);

sTimeInputSystem_t* INIT_TIME_INPUT_SUBSYSTEM(sTimeInputSystemAddresses_t timeInputSystemAddrInfo, uint32_t sysClockFrequency);

void CLEANUP_TIME_INPUT_SUBSYSTEM();

/*
 * sets all Time input channel registers from the values stored in pTimeChannel
 * returns 0 on success
 */
int setup_time_input_channel(sTimeInputSystem_t *pTimeInSystem);

int start_time_input_channel(sTimeInputSystem_t *pTimeInSystem);

int stop_time_input_channel(sTimeInputSystem_t *pTimeInSystem);

int reset_time_input_channel(sTimeInputSystem_t *pTimeInSystem);

int get_current_time_input_source(sTimeInputSystem_t *pTimeInSystem, eTimeInputSource_t* src);

/*
 * Gets the CH6 health for the Time input channel.
 */
uint32_t get_time_input_channel_health(sTimeInputSystem_t *pTimeInSystem);

int get_time_input_channel_time(sTimeInputSystem_t *pTimeInSystem, struct timespec * ts);

uint64_t time_input_get_current_rtc(sTimeInputSystem_t *pTimeInSystem);

uint64_t time_input_get_latched_rtc(sTimeInputSystem_t *pTimeInSystem);

int debug_log_time(sTimeInputSystem_t *pTimeInSystem);

int run_sysclk_cal(sTimeInputSystem_t *pTimeInSystem);

#endif /* TIME_INPUT_H_ */
