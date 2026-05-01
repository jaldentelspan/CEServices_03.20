/*
 * time_input.c
 *
 *  Created on: Oct 22, 2019
 *      Author: eric
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "time_input.h"
/* #include "si5157.h" */

const char *eTimeInputSourceString[] =
{
	"IRIG-AM",
	"IRIG-DC",
	"GPS",
	"1588",
	"EXT-1PPS",
	"RSVD5",
	"RSVD6",
	"INTERNAL"
};

void time_input_debug_print(sTimeInputSystem_t *pTimeInSystem)
{
	if(NULL == pTimeInSystem) return;

	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeSelectorBaseAddr)
	{
		tsd_time_input_selector_debug_print(pTimeInSystem->addrInfo.timeSelectorBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr)
	{
		tsd_manual_inc_time_debug_print(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeRTCCalBaseAddr)
	{
		tsd_rtc_calibrator_debug_print(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigAMCoreBaseAddr) )
	{
		tsd_rx_irig_core_debug_print(pTimeInSystem->addrInfo.irigAMCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigDCCoreBaseAddr) )
	{
		tsd_rx_irig_core_debug_print(pTimeInSystem->addrInfo.irigDCCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.gpsBaseAddr) )
	{
		tsd_gps_core_debug_print(pTimeInSystem->addrInfo.gpsBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr) )
	{
		tsd_rx_1pps_core_debug_print(pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.external1PPSCoreBaseAddr) )
	{
		tsd_rx_1pps_core_debug_print(pTimeInSystem->addrInfo.external1PPSCoreBaseAddr);
	}
}

int enable_all_time_input_components(sTimeInputSystem_t *pTimeInSystem)
{
	if(NULL == pTimeInSystem) return 1;

	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr)
	{
		tsd_manual_inc_time_enable(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigAMCoreBaseAddr) &&
			(pTimeInSystem->irig_am_enabled))
	{
		tsd_rx_irig_core_enable(pTimeInSystem->addrInfo.irigAMCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigDCCoreBaseAddr) &&
			(pTimeInSystem->irig_dc_enabled))
	{
		tsd_rx_irig_core_enable(pTimeInSystem->addrInfo.irigDCCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.gpsBaseAddr) &&
			(pTimeInSystem->gps_enabled))
	{
		tsd_gps_core_enable(pTimeInSystem->addrInfo.gpsBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr) &&
			(pTimeInSystem->ieee_1588_slave_enabled))
	{
		tsd_rx_1pps_core_enable(pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr);
	}
	if((UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.external1PPSCoreBaseAddr) &&
			(pTimeInSystem->external_1PPS_slave_enabled))
	{
		tsd_rx_1pps_core_enable(pTimeInSystem->addrInfo.external1PPSCoreBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeRTCCalBaseAddr)
	{
		tsd_rtc_calibrator_enable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);

	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeSelectorBaseAddr)
	{
		tsd_time_input_selector_enable(pTimeInSystem->addrInfo.timeSelectorBaseAddr);
	}
	return 0;
}

int disable_all_time_input_components(sTimeInputSystem_t *pTimeInSystem)
{
	if(NULL == pTimeInSystem) return 1;

	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeSelectorBaseAddr)
	{
		// for now, don't disable
//		tsd_time_input_selector_disable(pTimeInSystem->addrInfo.timeSelectorBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr)
	{
		// for now, don't disable this one
//		tsd_manual_inc_time_disable(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigAMCoreBaseAddr)
	{
		tsd_rx_irig_core_disable(pTimeInSystem->addrInfo.irigAMCoreBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigDCCoreBaseAddr)
	{
		tsd_rx_irig_core_disable(pTimeInSystem->addrInfo.irigDCCoreBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.gpsBaseAddr)
	{
		tsd_gps_core_disable(pTimeInSystem->addrInfo.gpsBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr)
	{
		tsd_rx_1pps_core_disable( pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr);
	}
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.external1PPSCoreBaseAddr)
	{
		tsd_rx_1pps_core_disable( pTimeInSystem->addrInfo.external1PPSCoreBaseAddr);
	}
	return 0;
}

int reset_all_time_input_components(sTimeInputSystem_t *pTimeInSystem)
{
	int val = 0;
	if(NULL == pTimeInSystem) return 1;
	disable_all_time_input_components(pTimeInSystem);
	usleep(1000);
	enable_all_time_input_components(pTimeInSystem);
	return val;
}


sTimeInputSystem_t* INIT_TIME_INPUT_SUBSYSTEM(sTimeInputSystemAddresses_t timeInputSystemAddrInfo, uint32_t sysClockFrequency)
{
	int itmp;
	uint32_t utmp;
//	uint16_t stmp;
	uint8_t btmp;
	sTimeInputSystem_t * pTimeInSystem = (sTimeInputSystem_t *)malloc(sizeof(sTimeInputSystem_t));
	if(pTimeInSystem)
	{
		memset(pTimeInSystem, 0, sizeof(sTimeInputSystem_t));
		pTimeInSystem->addrInfo = timeInputSystemAddrInfo;

		pTimeInSystem->qysModulesClkFrequency = sysClockFrequency;

		// make sure all components are disabled during init
		disable_all_time_input_components(pTimeInSystem);

		/*
		 * Manual Incrementer settings Settings
		 */
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr)
		{
			// none at this time
		}

		/*
		 * RX IRIG settings
		 */
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigAMCoreBaseAddr)
		{
			tsd_rx_irig_core_set_signal_type(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, TSD_RX_IRIG_SIGNAL_AM);
			tsd_rx_irig_core_get_code_type(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, &(pTimeInSystem->irig_am_code_type));
		}
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigDCCoreBaseAddr)
		{
			tsd_rx_irig_core_set_signal_type(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, TSD_RX_IRIG_SIGNAL_TTL);
			tsd_rx_irig_core_get_code_type(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, &(pTimeInSystem->irig_dc_code_type));
		}
		/*
		 * GPS settings
		 */
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.gpsBaseAddr)
		{
			tsd_gps_core_get_serial_baudrate(pTimeInSystem->addrInfo.gpsBaseAddr, pTimeInSystem->qysModulesClkFrequency, &itmp);
			if((itmp == (int)GPS_BAUDRATE_4800) ||
					(itmp == (int)GPS_BAUDRATE_9600) ||
					(itmp == (int)GPS_BAUDRATE_19200) ||
					(itmp == (int)GPS_BAUDRATE_38400) ||
					(itmp == (int)GPS_BAUDRATE_57600) ||
					(itmp == (int)GPS_BAUDRATE_115200))
			{
				pTimeInSystem->gps_baudrate = (eGPSBaudRate_t)itmp;
			}
			else
			{
				pTimeInSystem->gps_baudrate = GPS_BAUDRATE_9600;
			}

			tsd_gps_core_get_power_flags(pTimeInSystem->addrInfo.gpsBaseAddr, &utmp);
			if(utmp & TSD_GPS_CONTROL_ANT_5P0V) pTimeInSystem->gps_antenna_voltage = GPS_ANTENNA_VOLTAGE_5P0V;
			else if(utmp & TSD_GPS_CONTROL_ANT_3P3V) pTimeInSystem->gps_antenna_voltage = GPS_ANTENNA_VOLTAGE_3P3V;
			else pTimeInSystem->gps_antenna_voltage = GPS_ANTENNA_VOLTAGE_0V;

			tsd_gps_core_get_current_leapsecond(pTimeInSystem->addrInfo.gpsBaseAddr, &btmp);
			pTimeInSystem->gps_leap_second_val = (int)btmp;

//			tsd_gps_core_set_pps_window_min(pTimeInSystem->addrInfo.gpsBaseAddr, (10000000-1000));
//			tsd_gps_core_set_pps_window_max(pTimeInSystem->addrInfo.gpsBaseAddr, (10000000+1000));
		}
		/*
		 * Time Selector Settings
		 */
		if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeSelectorBaseAddr)
		{
			tsd_time_input_selector_get_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
					TSD_TIME_INPUT_SRC_IRIG_AM, &(pTimeInSystem->irig_am_prio));
			tsd_time_input_selector_get_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
					TSD_TIME_INPUT_SRC_IRIG_DC,  &(pTimeInSystem->irig_dc_prio));
			tsd_time_input_selector_get_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
					TSD_TIME_INPUT_SRC_GPS,  &(pTimeInSystem->gps_prio));
			tsd_time_input_selector_get_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
					TSD_TIME_INPUT_SRC_1588,  &(pTimeInSystem->ieee_1588_prio));
			tsd_time_input_selector_get_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
					TSD_TIME_INPUT_SRC_EXTERNAL_1PPS,  &(pTimeInSystem->external_1pps_prio));


		}
		/*
		 * 1588 PPS Settings
		 */
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr)
		{
			// none at this time
		}
		/*
		 * external 1PPS + Manual Time Entry Settings
		 */
		if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.external1PPSCoreBaseAddr)
		{
			// none at this time
		}
	}
	return pTimeInSystem;
}

void CLEANUP_TIME_INPUT_SUBSYSTEM(sTimeInputSystem_t *pTimeInSystem)
{
	if(pTimeInSystem)
	{
		disable_all_time_input_components(pTimeInSystem);
		free(pTimeInSystem);
	}
}

/*
 * sets all Time input channel registers from the values stored in pTimeInSystem
 * returns 0 on success
 */
int setup_time_input_channel(sTimeInputSystem_t *pTimeInSystem)
{
	int val = 0;
	uint32_t utmp;

	if(NULL == pTimeInSystem) return 1;

	// make sure all components are disabled during setup
	disable_all_time_input_components(pTimeInSystem);

	/*
	 * Manual Incrementer settings Settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr)
	{
		tsd_manual_inc_time_set_reset_offset(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr, pTimeInSystem->man_inc_reset_offset);
	}

	/*
	 * RTC Calibrator Settings
	 */
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeRTCCalBaseAddr)
	{

		if(pTimeInSystem->rtc_cal_100mhz_cal_enabled)
		{
			tsd_rtc_calibrator_100MHz_cal_enable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);
			tsd_rtc_calibrator_pid_disable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);
		}
		else{
			tsd_rtc_calibrator_100MHz_cal_disable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);

			if(pTimeInSystem->rtc_cal_pid_enabled) tsd_rtc_calibrator_pid_enable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);
			else tsd_rtc_calibrator_pid_disable(pTimeInSystem->addrInfo.timeRTCCalBaseAddr);
		}

	}

	/*
	 * RX IRIG settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigAMCoreBaseAddr)
	{
		val += tsd_rx_irig_core_set_signal_type(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, TSD_RX_IRIG_SIGNAL_AM);
		val += tsd_rx_irig_core_set_code_type(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, pTimeInSystem->irig_am_code_type);
	}
	/*
	 * RX IRIG settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.irigDCCoreBaseAddr)
	{
		val += tsd_rx_irig_core_set_signal_type(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, TSD_RX_IRIG_SIGNAL_TTL);
		val += tsd_rx_irig_core_set_code_type(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, pTimeInSystem->irig_dc_code_type);
	}
	/*
	 * GPS settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.gpsBaseAddr)
	{
		val += tsd_gps_core_set_serial_baudrate(pTimeInSystem->addrInfo.gpsBaseAddr, pTimeInSystem->qysModulesClkFrequency, (int)pTimeInSystem->gps_baudrate);
		utmp = TSD_GPS_CONTROL_PWR_3P3V | TSD_GPS_CONTROL_PWR_5P0V; /* always on */
		if(pTimeInSystem->gps_enabled)
		{
			switch(pTimeInSystem->gps_antenna_voltage)
			{
			case GPS_ANTENNA_VOLTAGE_5P0V:
				utmp |= TSD_GPS_CONTROL_ANT_5P0V;
				break;
			case GPS_ANTENNA_VOLTAGE_3P3V:
				utmp |= TSD_GPS_CONTROL_ANT_3P3V;
				break;
			default:
				break;
			}
		}
		val += tsd_gps_core_set_power_flags(pTimeInSystem->addrInfo.gpsBaseAddr, utmp);
		val += tsd_gps_core_set_current_leapsecond(pTimeInSystem->addrInfo.gpsBaseAddr, (uint8_t)pTimeInSystem->gps_leap_second_val);
		val += tsd_gps_core_set_pps_window_min(pTimeInSystem->addrInfo.gpsBaseAddr, 9999800);
		val += tsd_gps_core_set_pps_window_max(pTimeInSystem->addrInfo.gpsBaseAddr, 10000200);
	}
	/*
	 * Time Selector Settings
	 */
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeSelectorBaseAddr)
	{
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_IRIG_AM, (pTimeInSystem->irig_am_enabled ? pTimeInSystem->irig_am_prio : 0xF));
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_IRIG_DC, (pTimeInSystem->irig_dc_enabled ? pTimeInSystem->irig_dc_prio : 0xF));
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_GPS, (pTimeInSystem->gps_enabled ? pTimeInSystem->gps_prio : 0x0F));
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_1588, (pTimeInSystem->ieee_1588_slave_enabled ? pTimeInSystem->ieee_1588_prio : 0x0F));
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_EXTERNAL_1PPS, (pTimeInSystem->external_1PPS_slave_enabled ? pTimeInSystem->external_1pps_prio : 0xF));
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_RSVD6, 0xF);
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_RSVD7, 0xF);
		tsd_time_input_selector_set_input_prio(pTimeInSystem->addrInfo.timeSelectorBaseAddr,
				TSD_TIME_INPUT_SRC_INTERNAL,  0xF);

	}
	/*
	 * 1588 PPS Settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr)
	{
		// none at this time
	}
	/*
	 * 1PPS + Manual Time Entry Settings
	 */
	if(UNUSED_BASE_ADDRESS !=pTimeInSystem->addrInfo.external1PPSCoreBaseAddr)
	{
		// none at this time
	}

	return val;
}

int start_time_input_channel(sTimeInputSystem_t *pTimeInSystem)
{
	return enable_all_time_input_components(pTimeInSystem);
}

int stop_time_input_channel(sTimeInputSystem_t *pTimeInSystem)
{
	return disable_all_time_input_components(pTimeInSystem);
}

int reset_time_input_channel(sTimeInputSystem_t *pTimeInSystem)
{
	return reset_all_time_input_components(pTimeInSystem);
}

int get_current_time_input_source(sTimeInputSystem_t *pTimeInSystem, eTimeInputSource_t* src)
{
	if(UNUSED_BASE_ADDRESS == pTimeInSystem->addrInfo.timeSelectorBaseAddr) return 0;
	uint8_t val, type;
	int result = tsd_time_input_selector_get_selected_input(pTimeInSystem->addrInfo.timeSelectorBaseAddr, &val);

	result += tsd_time_input_selector_get_selected_input_type(pTimeInSystem->addrInfo.timeSelectorBaseAddr, &type);

	if((0 == result) && (type != TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_INTERNAL) && (
			(val == (uint8_t)TSD_TIME_INPUT_SRC_IRIG_AM) ||
			(val == (uint8_t)TSD_TIME_INPUT_SRC_IRIG_DC) ||
			(val == (uint8_t)TSD_TIME_INPUT_SRC_GPS) ||
			(val == (uint8_t)TSD_TIME_INPUT_SRC_1588) ||
			(val == (uint8_t)TSD_TIME_INPUT_SRC_EXTERNAL_1PPS)
			) )
	{
		*src = (eTimeInputSource_t)val;
	}
	else
	{
		*src = TSD_TIME_INPUT_SRC_INTERNAL;
	}

	return result;
}

/*
 * Gets the CH6 health for the Time input channel.
 */
uint32_t get_time_input_channel_health(sTimeInputSystem_t *pTimeInSystem)
{
	eTimeInputSource_t curInSrc;
	uint32_t health = 0;

	if(get_current_time_input_source(pTimeInSystem, &curInSrc))
	{
		health = TIMECODEHEALTH_SYNCHRONIZE_FAILURE | TIMECODEHEALTH_NO_EXTERNAL_SIGNAL;
		return health;
	}

	switch(curInSrc)
	{
	case TSD_TIME_INPUT_SRC_INTERNAL:
		health = TIMECODEHEALTH_SYNCHRONIZE_FAILURE | TIMECODEHEALTH_NO_EXTERNAL_SIGNAL;
		break;
	case TSD_TIME_INPUT_SRC_IRIG_AM:
		tsd_rx_irig_core_get_status_flags(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, &health);
		break;
	case TSD_TIME_INPUT_SRC_IRIG_DC:
		tsd_rx_irig_core_get_status_flags(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, &health);
		break;
	case TSD_TIME_INPUT_SRC_GPS:
		tsd_gps_core_get_status_flags(pTimeInSystem->addrInfo.gpsBaseAddr, &health);
		break;
	case TSD_TIME_INPUT_SRC_1588:// FIXME: ADD 1588 IN HERE WHEN WE HAVE IT
		tsd_rx_1pps_core_get_status_flags(pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr, &health);
		break;
	case TSD_TIME_INPUT_SRC_EXTERNAL_1PPS:// FIXME: ADD EXTERNAL 1PPS IN HERE WHEN WE HAVE IT
		tsd_rx_1pps_core_get_status_flags(pTimeInSystem->addrInfo.external1PPSCoreBaseAddr, &health);
		break;
	default:
		health = TIMECODEHEALTH_SYNCHRONIZE_FAILURE | TIMECODEHEALTH_NO_EXTERNAL_SIGNAL;
		break;
	}
	return health;
}

#define DO_IT_THE_EASY_WAY 1

int get_time_input_channel_time(sTimeInputSystem_t *pTimeInSystem, struct timespec * ts)
{
#if DO_IT_THE_EASY_WAY
	return tsd_manual_inc_time_get_time_ts(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr, ts);
#else
	eTimeInputSource_t curInSrc;

	if(get_current_time_input_source(pTimeInSystem, &curInSrc))
	{
		return 1;
	}

	switch(curInSrc)
	{
	case TSD_TIME_INPUT_SRC_INTERNAL:
		tsd_manual_inc_time_get_time_ts(pTimeInSystem->addrInfo.manualIncrementTimeBaseAddr, ts); // this is not right
		break;
	case TSD_TIME_INPUT_SRC_IRIG_AM:
		tsd_rx_irig_core_get_time_ts(pTimeInSystem->addrInfo.irigAMCoreBaseAddr, ts);
		break;
	case TSD_TIME_INPUT_SRC_IRIG_DC:
		tsd_rx_irig_core_get_time_ts(pTimeInSystem->addrInfo.irigDCCoreBaseAddr, ts);
		break;
	case TSD_TIME_INPUT_SRC_GPS:
		tsd_gps_core_get_time_ts(pTimeInSystem->addrInfo.gpsBaseAddr, ts);
		break;
	case TSD_TIME_INPUT_SRC_1588:// FIXME: ADD 1588 IN HERE WHEN WE HAVE IT
		tsd_rx_1pps_core_get_time_ts(pTimeInSystem->addrInfo.ptp1PPSCoreBaseAddr, ts);
		break;
	case TSD_TIME_INPUT_SRC_EXTERNAL_1PPS:// FIXME: ADD EXTERNAL 1PPS IN HERE WHEN WE HAVE IT
		tsd_rx_1pps_core_get_time_ts(pTimeInSystem->addrInfo.external1PPSCoreBaseAddr, ts);
		break;
	default:
		return 1;
		break;
	}
	return 0;
#endif
}

uint64_t time_input_get_current_rtc(sTimeInputSystem_t *pTimeInSystem)
{
	uint64_t rtc = 0;
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeRTCCalBaseAddr)
	{
		tsd_rtc_calibrator_get_current_rtc(pTimeInSystem->addrInfo.timeRTCCalBaseAddr, &rtc);
	}
	return rtc;
}

uint64_t time_input_get_latched_rtc(sTimeInputSystem_t *pTimeInSystem)
{
	uint64_t rtc = 0;
	if(UNUSED_BASE_ADDRESS != pTimeInSystem->addrInfo.timeRTCCalBaseAddr)
	{
		tsd_rtc_calibrator_get_latched_rtc(pTimeInSystem->addrInfo.timeRTCCalBaseAddr, &rtc);
	}
	return rtc;
}


