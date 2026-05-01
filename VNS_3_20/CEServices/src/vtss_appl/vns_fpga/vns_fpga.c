/*
 * vns_fpga.c
 *
 *  Created on: Mar 21, 2013
 *      Author: eric
 */

//#define DEBUG
#include "vtss_tod_api.h"
#include "vtss_api.h"
#include "time.h"
#include "vns_fpga_api.h"
#include "conf_api.h"
#ifdef VTSS_SW_OPTION_VCLI
#include "vns_fpga_cli.h"
#endif
#include "misc_api.h"
#include "vns_types.h"
#include "vns_registers.h"
#include "vtss_timer_api.h"
#include "port_api.h"
#include "firmware_api.h"

//#ifndef DEBUG
#include "ptp_api.h"
#include "unistd.h"
#include "critd_api.h"
#include "vtss_ptp_types.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_tod_api.h"
#include "vns_fpga_ptp.h"
#include <cyg/kernel/kapi.h>
#include "vtss_ecos_mutex_api.h"
//#endif

#include "conf_api.h"       /* for conf_board_t			                        */
#include <pthread.h>
#include "vns_pll.h"
#include "vns_epe.h"

/* #include "cli_api.h" */
/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include "vtss_module_id.h"
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>
#include <sys/time.h>

#define CYG_MSEC2TICK(msec) MAX(1, msec / (CYGNUM_HAL_RTC_NUMERATOR / CYGNUM_HAL_RTC_DENOMINATOR / 1000000)) // returns at least 1 tick




const vns_register_info vns_reg_info[] = {
    [FPGA_IRIG_IN_CHANNEL_ID]        = {0x00, 0x0000FFFF, 0  },	// 0
    [FPGA_IRIG_IN_SRC]               = {0x00, 0x000F0000, 16 },
    [FPGA_IRIG_IN_ENABLE]            = {0x00, 0x00100000, 20 },
    [FPGA_IRIG_IN_SIGNAL]            = {0x00, 0x00600000, 21 },
    [FPGA_UPDATE_CHUNK_MODE_ENABLE]  = {0x00, 0x20000000, 29 },
    [FPGA_UPDATE_LOAD_FACTORY_IMG]   = {0x00, 0x40000000, 30 },
    [FPGA_UPDATE_FACTORY_IMG]        = {0x00, 0x80000000, 31 },
    [FPGA_IRIG_IN_TIME_MSEC]         = {0x01, 0x000000FF, 0  },
    [FPGA_IRIG_IN_TIME_SEC]          = {0x01, 0x0000FF00, 8  },
    [FPGA_IRIG_IN_TIME_MIN]          = {0x01, 0x00FF0000, 16 },
    [FPGA_IRIG_IN_TIME_HOUR]         = {0x01, 0xFF000000, 24 },  /* 10 */
    [FPGA_IRIG_IN_TIME_DAY]          = {0x02, 0x0000000F, 0  },
    [FPGA_IRIG_IN_TIME_TDAY]         = {0x02, 0x00000FF0, 4  },
    [FPGA_IRIG_IN_TIME_YEAR]         = {0x02, 0x00FF0000, 16 },
    [FPGA_IRIG_IN_HEALTH]            = {0x02, 0x1C000000, 26 },
    [FPGA_UPDATE_STATUS]             = {0x02, 0x03000000, 24 },
    [FPGA_UPDATE_FACTORY]            = {0x02, 0x20000000, 29 },
    [FPGA_UPDATE_DONE]               = {0x02, 0x40000000, 30 },
    [FPGA_UPDATE_ERASE_DONE]         = {0x02, 0x80000000, 31 },
    [FPGA_IRIG_IN_RTC_CAL_LSB]       = {0x03, 0xFF000000, 24 }, //UNUSED
    [FPGA_IRIG_IN_RTC_DIFF_LSW]      = {0x04, 0xFFFFFF00, 8  },	/* 20 */
    [FPGA_IRIG_IN_CAL_MSB]           = {0x05, 0x000000FF, 0  },
    [FPGA_IRIG_IN_CAL_LSB]           = {0x05, 0x0000FF00, 8  },
    [FPGA_1588_IN_CAL_MSB]           = {0x05, 0x00FF0000, 16 }, // 1588 MSB
    [FPGA_1588_IN_CAL_LSB]           = {0x05, 0xFF000000, 24 }, // 1588 LSB
    [FPGA_IRIG_IN_OFFSET_MULTIPLIER] = {0x06, 0x000000FF, 0  },
    [FPGA_IRIG_IN_GPS_SECOND_WRITE]  = {0x06, 0x0000FF00, 8  },
    [FPGA_IRIG_IN_GPS_SECOND_READ]   = {0x06, 0x00FF0000, 16 },
    [FPGA_IRIG_IN_RTC_VAL_LSW]       = {0x07, 0xFFFFFFFF, 0  },
    [FPGA_IRIG_IN_RTC_VAL_MSW]       = {0x08, 0x0000FFFF, 0  }, 		//?? NEEDS ONE MORE BYTE
    [FPGA_IRIG_IN_INTERRUPT]         = {0x09, 0x00000001, 0  },
    [FPGA_FIRMWARE_VERSION_MINOR]    = {0x0A, 0x0000000F, 0  },
    [FPGA_FIRMWARE_VERSION_MAJOR]    = {0x0A, 0x000000F0, 4  },	/* 30 */
    [FPGA_IRIG_OUT_CHANNEL_ID]       = {0x0B, 0x0000FFFF, 0  },
    [FPGA_IRIG_OUT_TIMECODE]         = {0x0B, 0x00070000, 16 },
    [FPGA_IRIG_OUT_ENABLE]           = {0x0B, 0x00100000, 20 },
    [FPGA_IEEE_1588_MODE]            = {0x0B, 0x00600000, 21 },
    [FPGA_IRIG_OUT_MODE]             = {0x0B, 0x01800000, 23 },
    [FPGA_TTL_1PPS_ENABLE]           = {0x0B, 0x02000000, 25 },
    [FPGA_UPDATE_DATA]               = {0x0C, 0xFFFFFFFF, 0  },
    [FPGA_IRIG_OUT_HEALTH]           = {0x0D, 0x000000FF, 0  },
    [FPGA_IRIG_OUT_RTC_LSW]          = {0x0D, 0xFFFFFF00, 8  },
    [FPGA_IRIG_OUT_RTC_MSW]          = {0x0E, 0x00FFFFFF, 0  },	/* 40 */
    [FPGA_IRIG_OUT_PHASE_MULT0]      = {0x0F, 0x000000FF, 0  },
    [FPGA_IRIG_OUT_PHASE_MULT1]      = {0x0F, 0x0000FF00, 8  },
    [FPGA_IRIG_OUT_TCG_CAL_COARSE]   = {0x0F, 0x00FF0000, 16 },
    [FPGA_IRIG_OUT_TCG_CAL_MID]      = {0x0F, 0xFF000000, 24 },
    [FPGA_IRIG_OUT_TCG_CAL_FINE_MSB] = {0x10, 0x000000FF, 0  },
    [FPGA_IRIG_OUT_TCG_CAL_FINE_LSB] = {0x10, 0x0000FF00, 8  },
    [FPGA_IRIG_OUT_TIME_MSEC]        = {0x11, 0x000000FF, 0  },
    [FPGA_IRIG_OUT_TIME_SEC]         = {0x11, 0x0000FF00, 8  },
    [FPGA_IRIG_OUT_TIME_MIN]         = {0x11, 0x00FF0000, 16 },
    [FPGA_IRIG_OUT_TIME_HOUR]        = {0x11, 0xFF000000, 24 },	/* 50 */
    [FPGA_IRIG_OUT_TIME_DAY]         = {0x12, 0x0000FFFF, 0  },
    [FPGA_DISC_IN_STATE]             = {0x13, 0x0000001F, 0  },
    [FPGA_DISC_OUT_STATE]            = {0x14, 0x00001FFF, 0  },
    [FPGA_LED_CONTROL_R_B]           = {0x15, 0xFFFFFFFF, 0  },
    [FPGA_LED_CONTROL_G]             = {0x16, 0xF0FFFFFF, 0  },
    [FPGA_LED_CONTROL_FLASH]         = {0x17, 0xFFFFFFFF, 0  },
    [FPGA_1588_TIME_MS]              = {0x18, 0x0000000F, 0  },
    [FPGA_1588_TIME_SEC]             = {0x18, 0x0000FF00, 8  },
    [FPGA_1588_TIME_MIN]             = {0x18, 0x00FF0000, 16 },
    [FPGA_1588_TIME_HOUR]            = {0x18, 0xFF000000, 24 },	/* 60 */
    [FPGA_1588_TIME_HDAY]            = {0x19, 0x0000000F, 0  },
    [FPGA_1588_TIME_DAY]             = {0x19, 0x0000FF00, 8  },
    [FPGA_1588_TIME_YEAR]            = {0x19, 0x00FF0000, 16 },
    [FPGA_EPE_PLL_CTRL_ADDR]         = {0x1A, 0x0000003F, 0  },
    [FPGA_EPE_PLL_CTRL_RD]           = {0x1A, 0x00000080, 7  }, /* set to 1 then 0 */
    [FPGA_EPE_PLL_CTRL_WR]           = {0x1A, 0x00000100, 8  }, /* set to 1 then 0 */
    [FPGA_EPE_PLL_WR_DATA]           = {0x1B, 0xFFFFFFFF, 0  },
    [FPGA_EPE_PLL_RD_DATA]           = {0x1C, 0xFFFFFFFF, 0  },
    [FPGA_EPE_CH7_FRAME_SIZE]       = {0x1D, 0x0000000F, 0  },
    [FPGA_EPE_HDLC_ENABLE]          = {0x1D, 0x00000010, 4  },	/* 70 */
    [FPGA_EPE_HDLC_SEND_ADDR]       = {0x1D, 0x00000020, 5  },
    [FPGA_EPE_HDLC_CH7_MODE_MUX]    = {0x1D, 0x00000040, 6  },
    [FPGA_EPE_CH7_ENABLE]           = {0x1D, 0x00000080, 7  },
    [FPGA_EPE_HDLC_ADDR_BYTE]       = {0x1D, 0x0000FF00, 8  },
    [FPGA_EPE_HDLC_CTRL_BYTE]       = {0x1D, 0x00FF0000, 16 },
    [FPGA_EPE_CTRL_DEC_CH7_FRM_SZ]  = {0x1D, 0x0F000000, 24 },
    [FPGA_EPE_CTRL_DEC_HDLC_ENABLE] = {0x1D, 0x10000000, 28 },
    [FPGA_EPE_CTRL_DEC_CH7_ENABLE]  = {0x1D, 0x20000000, 29 },
    [FPGA_LED_CONTROL2_R]            = {0x1E, 0xF0000FFF, 0  },
    [FPGA_LED_CONTROL2_B]            = {0x1E, 0x0FFFF000, 0  },
    [FPGA_LED_CONTROL2_G]            = {0x1F, 0xF0FFFFFF, 0  },
    [FPGA_LED_CONTROL2_FLASH]        = {0x20, 0xF7FFFFFF, 0  },
    [FPGA_LED_CONTROL2_MODE]         = {0x20, 0x08000000, 27 },

    [FPGA_REGISTER_END]              = {0x00, 0x00000000, 0  }
};
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_VNS_FPGA
#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_IRQ     1
#define VTSS_TRACE_GRP_VNS_BASE_TIMER 2
#define TRACE_GRP_CNT          3

#define FPGA_UPDATE_FAILURE_STRING_MAX_SIZE (100)
typedef struct fpga_update_info{
	update_args_t file_info;
	int disable_timer;
	int must_update;
	int is_duplicating; /* Being shared by threads web thread waits for dupliction to complete*/
	vtss_ecos_mutex_t mutex;
} fpga_update_info_t;

static struct{
	int init_complete;
	critd_t registermutex;
	int registermutex_lock;
	fpga_update_info_t fpga_update;
	vtss_timer_t ptp_timer;
	vtss_timer_t led_timer;
        int disable_i2c;
        /* added to track port activity to control LED's 3627 & 3638 */
        u32 link;
        u32 activity;
        u32 speed;

        char fpga_update_failure_string[FPGA_UPDATE_FAILURE_STRING_MAX_SIZE];

        
    /* keeping conf for quick access */
    /* it will update on reboot */
    conf_board_t board_conf;
    u8 fw_major;
    u8 fw_minor;
} vns_global;

//TODO: Remove this for build...
#ifndef NULL
#define NULL (void*)0
#endif

#define PTP_TIMEOFFSET 1

/* #define IES_DEBUGGING */
/* #define IES_DEBUGGING_EPE */

#if (VTSS_TRACE_ENABLED)
// Trace registration. Initialized by vns_fpga_init() */
static vtss_trace_reg_t trace_reg = {
  .module_id = VTSS_TRACE_MODULE_ID,
  .name      = "vns_fpga",
  .descr     = "VNS FPGA module"
};

#ifndef VTSS_VNS_FPGA_DEFAULT_TRACE_LVL
#define VTSS_VNS_FPGA_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

#define LOCK_TRACE_LEVEL VTSS_TRACE_LVL_NOISE

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
  [VTSS_TRACE_GRP_DEFAULT] = {
    .name      = "default",
    .descr     = "Default",
#ifdef IES_DEBUGGING
    .lvl       = VTSS_TRACE_LVL_DEBUG,
#else
    .lvl       = VTSS_TRACE_LVL_WARNING,
#endif /* IES_DEBUGGING */
    .timestamp = 1,
  },
  [VTSS_TRACE_GRP_IRQ] = {
    .name      = "IRQ",
    .descr     = "IRQ",
    .lvl       = VTSS_VNS_FPGA_DEFAULT_TRACE_LVL,
    .timestamp = 1,
    .irq       = 1,
    .ringbuf   = 1,
  },
  [VTSS_TRACE_GRP_VNS_BASE_TIMER] = {
      .name      = "timer",
      .descr     = "VNS FPGA Internal timer",
    .lvl       = VTSS_TRACE_LVL_WARNING,
      .timestamp = 1,
      .usec = 1,
  },
};
// TODO: REMOVED MUTEX (MAYBE THE TIMER USES THE SAME THREAD???)
#define FPGA_REG_LOCK()        critd_enter(&vns_global.registermutex, VTSS_TRACE_GRP_DEFAULT, LOCK_TRACE_LEVEL, __FILE__, __LINE__)
#define FPGA_REG_UNLOCK()      critd_exit (&vns_global.registermutex, VTSS_TRACE_GRP_DEFAULT, LOCK_TRACE_LEVEL, __FILE__, __LINE__)
/* #define FPGA_REG_LOCK() 		do{}while(0) */
/* #define FPGA_REG_UNLOCK() 		do{}while(0) */
#else
#define FPGA_REG_LOCK() 		do{}while(0)
#define FPGA_REG_UNLOCK() 		do{}while(0)
#endif /* VTSS_TRACE_ENABLED */


static const char *fpga_firmware_status = "idle";
/*
 * sets flag to be cleared by fpga_update_base_thread in start_fpga_upgrade
 * file must have time for allocation
 * Used in conjuction with is_fpga_udate_ready
 */
void set_fpga_update_mode(update_args_t* args);
/*
 * used with set_fpga_update_mode()
 * no need to be pulic
 */
void clear_fpga_update_mode(void);
int calculate_checksum32_buffer(unsigned int* checksum,  void * file_ptr, size_t file_length);
/*
 * Need to cancel timer for Firmware update, traffic to the FPGA.
 * also to set factory defaults.  Crash occurs otherwise.
 */
static void cancel_timer(void);
static void init_timer(void);
static void init_setup_timer(void);
static void init_led_discrete_timer(void);
static void init_debug_timer(void);
int Time_t_2_struct_tm_time(time_t t, struct tm * ptime);
/* int Time_t_2_Vns_bcd_time(time_t t, vns_bcd_time * bcd); */

static vns_fpga_conf_t config_shaddow;
vns_status_t vns_status;
//static BOOL i2c_busy;
BOOL debug_test_mode; // used to turn off automatic writes to the FPGA (when set to true)

static BOOL ptp_lock = FALSE;
static vtss_timestamp_t ptp_time;
static int fail_count;

#define ALLOW_PTP_CLOCK_SETUP 1

void delete_ptp_clock()
{
#if ALLOW_PTP_CLOCK_SETUP
	ptp_clock_default_ds_t clock_bs;
	vtss_ptp_ext_clock_mode_t mode;
	vtss_timestamp_t vt;
	ptp_clock_slave_ds_t clock_slave_bs;
	port_iter_t pit;
	vtss_port_no_t port_no;
	int inst = 0;
	if(status_ptp(&clock_bs, &mode, &vt, &clock_slave_bs))
	{
		// clocd does not exist... nothing to do
	}
	else
	{
		//delete instance 0
		ptp_clock_delete(inst);
		if ((ptp_get_clock_default_ds(&clock_bs,inst))) {
			port_iter_init_local(&pit);
			while (port_iter_getnext(&pit)) {
				port_no = pit.iport;
				ptp_port_dis(iport2uport(port_no), inst);
			}
		}
	}
#endif /* ALLOW_PTP_CLOCK_SETUP */
}

void create_ptp_clock(BOOL master)
{
#if ALLOW_PTP_CLOCK_SETUP

	ptp_clock_default_ds_t clock_bs;
    ptp_set_clock_ds_t set_clock_bs;
    ptp_port_ds_t port_bs;
    ptp_set_port_ds_t  set_port_bs;
	ptp_clock_slave_ds_t clock_slave_bs;
	vtss_ptp_ext_clock_mode_t mode;
	ptp_clock_timeproperties_ds_t clock_time_properties_bs;
    vtss_ptp_default_servo_config_t default_servo;
    vtss_ptp_default_delay_filter_config_t default_delay;
    vtss_ptp_default_filter_config_t default_offset;
	vtss_timestamp_t vt;
	port_iter_t pit;
	vtss_port_no_t port_no;
	int inst = 0;

    ptp_init_clock_ds_t clock_init_bs;
    mac_addr_t my_sysmac;

    memset(&clock_init_bs,0,sizeof(clock_init_bs));
    memset(&clock_bs,0,sizeof(clock_bs));
    memset(&port_bs,0,sizeof(port_bs));

    vtss_ext_clock_out_get(&mode);
//    if((master && (mode.one_pps_mode == VTSS_PTP_ONE_PPS_INPUT)) ||
//    		(!master && (mode.one_pps_mode == VTSS_PTP_ONE_PPS_OUTPUT)))
//    {
//    	//we are already configured... do nothing here
//    }
//    else
//    {
		if(master)
			mode.one_pps_mode = VTSS_PTP_ONE_PPS_INPUT;
		else
			mode.one_pps_mode = VTSS_PTP_ONE_PPS_OUTPUT;
		mode.clock_out_enable = FALSE;
		mode.vcxo_enable = FALSE;
		mode.freq = 1;

		vtss_ext_clock_out_set(&mode);
//    }

	if(status_ptp(&clock_bs, &mode, &vt, &clock_slave_bs))
	{//failed
		//T_E("VNS_FGA: Failed to get ptp status");
		//this means that instance 0 does not exist...need to create it...
		//Now create a new instance 0
		clock_init_bs.deviceType = PTP_DEVICE_ORD_BOUND;
	    clock_init_bs.twoStepFlag =  TRUE;

	    conf_mgmt_mac_addr_get(my_sysmac, 0);
	    clock_init_bs.clockIdentity[0] = my_sysmac[0];
	    clock_init_bs.clockIdentity[1] = my_sysmac[1];
	    clock_init_bs.clockIdentity[2] = my_sysmac[2];
	    clock_init_bs.clockIdentity[3] = 0xff;
	    clock_init_bs.clockIdentity[4] = 0xfe;
	    clock_init_bs.clockIdentity[5] = my_sysmac[3];
	    clock_init_bs.clockIdentity[6] = my_sysmac[4];
	    clock_init_bs.clockIdentity[7] = (my_sysmac[5] + inst);

	    clock_init_bs.oneWay =  FALSE;
	    clock_init_bs.protocol = PTP_PROTOCOL_ETHERNET;
		/* VLAN Tagging support is enabled*/
		clock_init_bs.tagging_enable = FALSE;
	    clock_init_bs.configured_vid = 1;
	    clock_init_bs.configured_pcp = 0;
            clock_init_bs.mep_instance = 1;
	    if (!ptp_clock_create(&clock_init_bs, inst)) {
	//        T_D(" Cannot Create Clock: Tried to Create more than one transparent clock ");
	    }
	}
//	else
//	{
//		//delete instance 0
//		ptp_clock_delete(inst);
//		if ((ptp_get_clock_default_ds(&clock_bs,inst))) {
//			port_iter_init_local(&pit);
//			while (port_iter_getnext(&pit)) {
//				port_no = pit.iport;
//				ptp_port_dis(iport2uport(port_no), inst);
//			}
//		}
//	}

    /* Look for all the ports that have been configured
       for this new Clock */
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        port_no = pit.iport;
//		T_D("Enabling the port -[%d] ",port_no);
		if (!ptp_port_ena(FALSE, iport2uport(port_no), inst)) {
//			T_D("clock_instance not created -[%d] or timestamp feature not available", inst);
		}
    }

    if ((ptp_get_clock_default_ds(&clock_bs,inst))) {
        set_clock_bs.domainNumber = 0;
        set_clock_bs.priority1 = (master ? 100 : 200);
        set_clock_bs.priority2 = 128;
        if (!ptp_set_clock_default_ds(&set_clock_bs,inst)) {
           T_W("Clock instance %d : does not exist", inst);
        }
        if (ptp_get_clock_timeproperties_ds(&clock_time_properties_bs,inst)) {

			clock_time_properties_bs.currentUtcOffset = 0;
			clock_time_properties_bs.currentUtcOffsetValid = FALSE;
			clock_time_properties_bs.leap59 = FALSE;
			clock_time_properties_bs.leap61 = FALSE;
			clock_time_properties_bs.timeTraceable = FALSE;
			clock_time_properties_bs.frequencyTraceable = FALSE;
			clock_time_properties_bs.ptpTimescale = TRUE;
			if( (config_shaddow.time_in_setting.source == TIME_INPUT_SRC_GPS0) ||
					(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_GPS3) ||
					(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_GPS5) )
			{
				clock_time_properties_bs.timeSource = (master ? 32 : 160);
			}
			else if (config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1588)
			{
				clock_time_properties_bs.timeSource = (master ? 64: 160);
			}
			else
			{
				clock_time_properties_bs.timeSource = (master ? 144 : 160);
			}
			if (!ptp_set_clock_timeproperties_ds(&clock_time_properties_bs,inst)) {
				T_W("Clock instance %d : does not exist", inst);
			}
		}

		if (ptp_default_servo_parameters_get(&default_servo, inst)) {

			default_servo.display_stats =  FALSE;
			default_servo.p_reg =  TRUE;
			default_servo.i_reg =  TRUE;
			default_servo.d_reg =  TRUE;
			default_servo.ap = 3;
			default_servo.ai = 80;
			default_servo.ad = 40;

			if (!ptp_default_servo_parameters_set(&default_servo,inst)) {
				T_W("Clock instance %d : does not exist", inst);
			}
		}
		if (ptp_default_delay_filter_parameters_get(&default_delay, inst)) {

			default_delay.delay_filter  = 6;
			if (!ptp_default_delay_filter_parameters_set(&default_delay, inst)) {
				T_W("Clock instance %d : does not exist", inst);
			}
		}
		if (ptp_default_filter_parameters_get(&default_offset, inst)) {
				default_offset.period = 1;
				default_offset.dist = 2;
			if (!ptp_default_filter_parameters_set(&default_offset, inst)) {
				T_W("Clock instance %d : does not exist", inst);
			}
		}

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            port_no = pit.iport;
            if (ptp_get_port_ds(&port_bs, iport2uport(port_no), inst)) {
                set_port_bs.logAnnounceInterval = 1;
                set_port_bs.announceReceiptTimeout = 3;
                set_port_bs.logSyncInterval = -6;
                set_port_bs.delayMechanism = 1; //"e2e", (end to end)... 2 = "ptp" (point to point)
                set_port_bs.logMinPdelayReqInterval = 3;
                set_port_bs.delayAsymmetry = 0;
                set_port_bs.ingressLatency = 0;
                set_port_bs.egressLatency = 0;
                set_port_bs.versionNumber = 2;

				if (!ptp_set_port_ds(iport2uport(port_no), &set_port_bs,inst)) {
//					T_W("Clock instance %d : does not exist",inst);
				}
            }
        }
    }
#endif /* ALLOW_PTP_CLOCK_SETUP */
}

void set_config_shaddow(vns_fpga_conf_t cfg)
{
	int i;
	config_shaddow.version = cfg.version;
	config_shaddow.verbose = cfg.verbose;

	for(i = 0; i < DISCRETE_IN_MAX; i++)
	{
		config_shaddow.di_config.di_setting[i] = cfg.di_config.di_setting[i];
	}
	for(i = 0; i < DISCRETE_OUT_MAX; i++)
	{
		config_shaddow.do_config.do_setting[i] = cfg.do_config.do_setting[i];
	}
	config_shaddow.time_in_setting.channel_id = cfg.time_in_setting.channel_id;
	config_shaddow.time_in_setting.signal     = cfg.time_in_setting.signal;
	config_shaddow.time_in_setting.source     = cfg.time_in_setting.source;

	config_shaddow.time_out_setting.channel_id        = cfg.time_out_setting.channel_id;
	config_shaddow.time_out_setting.mode              = cfg.time_out_setting.mode;
	config_shaddow.time_out_setting.ieee_1588_type    = cfg.time_out_setting.ieee_1588_type;
	config_shaddow.time_out_setting.timecode          = cfg.time_out_setting.timecode;
	config_shaddow.time_out_setting.irig_dc_1pps_mode = cfg.time_out_setting.irig_dc_1pps_mode;
	config_shaddow.time_out_setting.leap_seconds      = cfg.time_out_setting.leap_seconds;

        config_shaddow.epe_config = cfg.epe_config; 
        T_D("epe_conf->mode %u", config_shaddow.epe_config.mode );
        T_D("epe_conf->bit_rate %u", config_shaddow.epe_config.bit_rate );
        T_D("epe_conf->ch7_data_word_count %u", config_shaddow.epe_config.ch7_data_word_count );
	/* config_shaddow.epe_config.mode                       = cfg.epe_config.mode; */
	/* config_shaddow.epe_config.bit_rate                   = cfg.epe_config.bit_rate; */
	/* config_shaddow.epe_config.ch7_data_word_count        = cfg.epe_config.ch7_data_word_count; */
	/* config_shaddow.epe_config.license_info.reserved1     = cfg.epe_config.license_info.reserved1 ; */
	/* config_shaddow.epe_config.license_info.reserved2     = cfg.epe_config.license_info.reserved2 ; */
	/* config_shaddow.epe_config.license_info.epe_signature = cfg.epe_config.license_info.epe_signature ; */
	/* config_shaddow.epe_config.license_info.ies_signature = cfg.epe_config.license_info.ies_signature ; */
	/* config_shaddow.epe_config.license_info.key           = cfg.epe_config.license_info.key ; */

	for(i = 0; i < TIME_IN_CAL_TABLE_ENTRIES; i++)
	{
		config_shaddow.time_in_calibration[i].cal = cfg.time_in_calibration[i].cal;
//		config_shaddow.time_in_calibration[i].small_cal = cfg.time_in_calibration[i].small_cal;
//		config_shaddow.time_in_calibration[i].rtc_cal = cfg.time_in_calibration[i].rtc_cal;
		config_shaddow.time_in_calibration[i].offset_multiplier = cfg.time_in_calibration[i].offset_multiplier;
	}
	for(i = 0; i < TIME_OUT_CAL_TABLE_ENTRIES; i++)
	{
		config_shaddow.time_out_calibration[i].coarse_cal = cfg.time_out_calibration[i].coarse_cal;
		config_shaddow.time_out_calibration[i].med_cal = cfg.time_out_calibration[i].med_cal;
		config_shaddow.time_out_calibration[i].fine_cal = cfg.time_out_calibration[i].fine_cal;
	}
}

vns_fpga_conf_t * get_shadow_config()
{
	return &config_shaddow;
}

#if defined(BOARD_IGU_REF)
static u32 fake_registers[FPGA_REGISTER_END];
#endif

#if defined(BOARD_IGU_REF)
int _GetRegister(vns_register_info reg_info, u32 * data)
{
	u32 wdata = fake_registers[reg_info.regnum];
	*data = ((wdata & reg_info.mask) >> reg_info.shift);
	return 0;
}
#else
int _GetRegister(vns_register_info reg_info, u32 * data)
{
    int error=0;
    uchar reg_addr_b[1];
    uchar fpga_i2c_data[4];
    u32 wdata = 0;

    reg_addr_b[0] = reg_info.regnum;
    FPGA_REG_LOCK();
    if (vtss_i2c_wr_rd(NULL, FPGA_I2C_ADDR, &reg_addr_b[0], 1, &fpga_i2c_data[0], 4, 100, NO_I2C_MULTIPLEXER) == VTSS_RC_OK)
    {
        wdata = (((u32)fpga_i2c_data[3] << 24) +
                ((u32)fpga_i2c_data[2] << 16) +
                ((u32)fpga_i2c_data[1] << 8) +
                ((u32)fpga_i2c_data[0]));
        //T_D("I^2C register read: %08X", wdata);
        //now mask and shift
        *data = ((wdata & reg_info.mask) >> reg_info.shift);
 #ifdef IES_DEBUGGING_EPE
        if(reg_info.regnum >  0x19 ) {
            /* debugging EPE */
            T_D("Getting Reg: 0x%02X data: 0x%02X %02X %02X %02X",
                    reg_info.regnum,
                    fpga_i2c_data[3],
                    fpga_i2c_data[2],
                    fpga_i2c_data[1],
                    fpga_i2c_data[0]);
        }
 #endif

    }
    else
    {
        /* Cannot return right here because of mutex */
        error=1;
    }
    FPGA_REG_UNLOCK();
    return error;
}
#endif

int GetRegister(vns_registers_t reg, u32 * data)
{
	vns_register_info reg_info;
	reg_info = vns_reg_info[reg];
        if(is_i2c_disabled())
            return 0;
	return _GetRegister(reg_info, data);
}

#if defined(BOARD_IGU_REF)
int _SetRegister(vns_register_info reg_info, u32 data)
{
	vns_register_info full_word;
	u32 set_data;


	full_word.regnum = reg_info.regnum;
	full_word.mask = 0xFFFFFFFF;
	full_word.shift = 0;

	//get the current register value...
	if(_GetRegister(full_word, &set_data))
	{
		return 1;
	}
	//clear the bits that we are about to set
	set_data &= ~reg_info.mask;

	//shift into its place and mask (just in case) then or it to our set_data
	set_data |= ((data << reg_info.shift) & reg_info.mask);

	fake_registers[reg_info.regnum] = set_data;

	return 0;
}
#else
int _SetRegister(vns_register_info reg_info, u32 data)
{
	vns_register_info full_word;
	u32 set_data;
        int error = 0;


	full_word.regnum = reg_info.regnum;
	full_word.mask = 0xFFFFFFFF;
	full_word.shift = 0;

	//get the current register value...
	if(_GetRegister(full_word, &set_data))
	{
		return 1;
	}

	//clear the bits that we are about to set
	set_data &= ~reg_info.mask;

	//shift into its place and mask (just in case) then or it to our set_data
	set_data |= ((data << reg_info.shift) & reg_info.mask);


	uchar fpga_i2c_data[5];
	fpga_i2c_data[0] = reg_info.regnum;
	fpga_i2c_data[1] = (set_data >> 0) & 0xFF;
	fpga_i2c_data[2] = (set_data >> 8) & 0xFF;
	fpga_i2c_data[3] = (set_data >> 16) & 0xFF;
	fpga_i2c_data[4] = (set_data >> 24) & 0xFF;
 #ifdef IES_DEBUGGING_EPE
        if(reg_info.regnum >  0x19 ) {
            /* debugging EPE */
            T_D("Setting Reg: 0x%02X data: 0x%02X %02X %02X %02X",
                    fpga_i2c_data[0],
                    fpga_i2c_data[4],
                    fpga_i2c_data[3],
                    fpga_i2c_data[2],
                    fpga_i2c_data[1]);
        }
 #endif
        FPGA_REG_LOCK();
        if (vtss_i2c_wr(NULL, FPGA_I2C_ADDR, &fpga_i2c_data[0], 5, 100, NO_I2C_MULTIPLEXER) == VTSS_RC_ERROR)
        {
            /* cannot return here because of mutex */
            error=1;
        }
        FPGA_REG_UNLOCK();
	return error;
}
#endif
int SetRegister(vns_registers_t reg, u32 data)
{
	vns_register_info reg_info;
	reg_info = vns_reg_info[reg];
        if(is_i2c_disabled())
            return 0;
	return _SetRegister(reg_info, data);
}

int Set_DiscreteInputConfig(discrete_in_config disc_in_cfg)
{
	int i;
	for(i = 0; i < DISCRETE_IN_MAX; i++)
		config_shaddow.di_config.di_setting[i] = disc_in_cfg.di_setting[i];
	return 0;
}

int Get_DiscreteInputConfig(discrete_in_config * disc_in_cfg)
{
	int i;
	for(i = 0; i < DISCRETE_IN_MAX; i++)
		disc_in_cfg->di_setting[i] = config_shaddow.di_config.di_setting[i];
	return 0;
}

int Set_DiscreteOutputConfig(discrete_out_config disc_out_cfg)
{
	int i;
	for(i = 0; i < DISCRETE_OUT_MAX; i++)
		config_shaddow.do_config.do_setting[i] = disc_out_cfg.do_setting[i];
	return 0;
}

int Get_DiscreteOutputConfig(discrete_out_config * disc_out_cfg)
{
	int i;
	for(i = 0; i < DISCRETE_OUT_MAX; i++)
		disc_out_cfg->do_setting[i] = config_shaddow.do_config.do_setting[i];
	return 0;
}

int Get_DiscreteInputState(u32 * di_state)
{
	return GetRegister(FPGA_DISC_IN_STATE, di_state);
}

int Set_GPSWrittenLeapValue(u16 leap_seconds)
{
    int retval = 0;
    //retval += Stop_TimeOutput();
    u32 val = leap_seconds & 0xFF;
    T_W("setting write leap reg: 0x%X: 0x%X val: 0x%X",FPGA_IRIG_IN_GPS_SECOND_WRITE, leap_seconds, val);
    retval += SetRegister(FPGA_IRIG_IN_GPS_SECOND_WRITE, val);
    return retval;
}

int save_leap_seconds(void)
{
    vns_fpga_conf_t * vns_cfg_blk;
    u32 size;
    /* Open or create VNS configuration block */
    vns_cfg_blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, &size);
    if ( (vns_cfg_blk == NULL) || (size != sizeof(*vns_cfg_blk)) ) {
        T_W("VNS_CONFIG: conf_sec_open failed or size mismatch, creating defaults");
        vns_cfg_blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, sizeof(*vns_cfg_blk));
    }

    if (vns_cfg_blk != NULL) {
        T_D("Saving leap Second");
        /* Use start values */
        vns_cfg_blk->time_out_setting.leap_seconds      = config_shaddow.time_out_setting.leap_seconds;

        T_D("Before conf_sec_close");
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF);
        T_D("After conf_sec_close");
    }
    else {
        T_E("VNS_CONFIG: conf_sec_open failed could not save VNS Configuration;");
        return 1;
    }
    return 0;
}

int Set_StoredLeapSeconds(u16 leap_seconds)
{
    int retval = 0;
    conf_mgmt_conf_t conf;
    conf_mgmt_conf_get(&conf);
    if(!conf.flash_save)
    {
        conf.flash_save = TRUE;
        conf_mgmt_conf_set(&conf);
        conf.flash_save = FALSE;
    }
    if(retval)
        T_D("Set_GPSWrittenLeapValue Failed: 0x%X", retval);
    /* T_D("Storing leap second in flash value: 0x%X", leap_seconds); */
    config_shaddow.time_out_setting.leap_seconds = leap_seconds;
    save_leap_seconds();
    conf_mgmt_conf_set(&conf);

    return retval;
}
int Get_StoredLeapSeconds(u16 * leap_seconds)
{
    int retval = 0;
    *leap_seconds = config_shaddow.time_out_setting.leap_seconds ;
    /* T_D("Getting Stored leap second : 0x%X in Struct 0x%X ", *leap_seconds, config_shaddow.time_out_setting.leap_seconds); */
    return retval;
}
/* int Get_GPSWrittenLeapValue(u16 * leap_seconds) */
/* { */
/*     int retval = 0; */
/*     u32 val; */
/*     retval += GetRegister(FPGA_IRIG_IN_GPS_SECOND_WRITE, &val); */
/*     if(retval == 0) */
/*     { */
/*         *leap_seconds = val & 0xFF; */
/*     } */
/*     return retval; */
/* } */

int Get_GPSRecievedLeapValue(u16 * leap_seconds)
{
	int retval = 0;
	u32 val;
	retval += GetRegister(FPGA_IRIG_IN_GPS_SECOND_READ, &val);
	if(retval == 0)
	{
		*leap_seconds = val & 0xFF;
	}
        /* T_D("Getting recieved leap second reg : 0x%X val: 0x%X 0x%X in Struct",vns_reg_info[FPGA_IRIG_IN_GPS_SECOND_READ].regnum, val, *leap_seconds); */
	return retval;
}

int Get_FPGAFirmwareVersionMajor(u8 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_FIRMWARE_VERSION_MAJOR, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0x0F;
	}
	return retval;
//	return GetRegister(FPGA_FIRMWARE_VERSION_MAJOR, fw_version);
}
int Get_FPGAFirmwareVersionMinor(u8 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_FIRMWARE_VERSION_MINOR, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0x0F;
	}
	return retval;
//	return GetRegister(FPGA_FIRMWARE_VERSION_MINOR, fw_version);
}
int Set_DiscreteOutputState(u32 do_state)
{
	return SetRegister(FPGA_DISC_OUT_STATE, do_state);
}

int Get_DiscreteOutputState(u32 * do_state)
{
	return GetRegister(FPGA_DISC_OUT_STATE, do_state);
}

int Set_TimeInCalibration(vns_time_in_cal time_in_cal)
{
	vns_time_input_src_t input_src;
	int retval = 0;
	switch(config_shaddow.time_in_setting.source)
	{
	case (TIME_INPUT_SRC_IRIG_B):
		input_src = TIME_INPUT_SRC_IRIG_B;
		break;
	case (TIME_INPUT_SRC_IRIG_A):
		input_src = TIME_INPUT_SRC_IRIG_A;
		break;
	case (TIME_INPUT_SRC_IRIG_G):
		input_src = TIME_INPUT_SRC_IRIG_G;
		break;
	case (TIME_INPUT_SRC_1588):
		input_src = TIME_INPUT_SRC_1588;
		break;
	case (TIME_INPUT_SRC_1PPS):
		input_src = TIME_INPUT_SRC_1PPS;
		break;
	case (TIME_INPUT_SRC_DISABLED):
		input_src = TIME_INPUT_SRC_DISABLED;
		break;
	default://it must be one of the GPS settings
		input_src = TIME_INPUT_SRC_GPS0;
		break;
	}
	if(input_src != TIME_INPUT_SRC_DISABLED)
	{
		//configure the shaddow.
		config_shaddow.time_in_calibration[input_src].cal = time_in_cal.cal;
		config_shaddow.time_in_calibration[input_src].offset_multiplier = time_in_cal.offset_multiplier;
		//set the new config.
		retval += SetRegister(FPGA_IRIG_IN_CAL_MSB, (time_in_cal.cal >> 8) );
		retval += SetRegister(FPGA_IRIG_IN_CAL_LSB, (time_in_cal.cal & 0xFF) );
		retval += SetRegister(FPGA_IRIG_IN_OFFSET_MULTIPLIER, time_in_cal.offset_multiplier);
	}
	return retval;
}

int Get_TimeInCalibration(vns_time_in_cal * time_in_cal)
{
	vns_time_input_src_t input_src;
	int retval = 0;
	u32 val;
	switch(config_shaddow.time_in_setting.source)
	{
	case (TIME_INPUT_SRC_IRIG_B):
		input_src = TIME_INPUT_SRC_IRIG_B;
		break;
	case (TIME_INPUT_SRC_IRIG_A):
		input_src = TIME_INPUT_SRC_IRIG_A;
		break;
	case (TIME_INPUT_SRC_IRIG_G):
		input_src = TIME_INPUT_SRC_IRIG_G;
		break;
	case (TIME_INPUT_SRC_1588):
		input_src = TIME_INPUT_SRC_1588;
		break;
	case (TIME_INPUT_SRC_1PPS):
		input_src = TIME_INPUT_SRC_1PPS;
		break;
	case (TIME_INPUT_SRC_DISABLED):
		input_src = TIME_INPUT_SRC_DISABLED;
		break;
	default://it must be one of the GPS settings
		input_src = TIME_INPUT_SRC_GPS0;
		break;
	}

	if(input_src == TIME_INPUT_SRC_DISABLED)
	{
		retval = 1;
	}
	else
	{
		retval += GetRegister(FPGA_IRIG_IN_CAL_MSB, &val);
		if(retval == 0)
		{
			time_in_cal->cal = val << 8;
		}
		retval += GetRegister(FPGA_IRIG_IN_CAL_LSB, &val);
		if(retval == 0)
		{
			time_in_cal->cal |= val;
		}
		retval += GetRegister(FPGA_IRIG_IN_OFFSET_MULTIPLIER, &val);
		if(retval == 0)
		{
			time_in_cal->offset_multiplier = val;
		}
	}
	if(retval == 0)
	{
		//make sure the shaddow is the same
		config_shaddow.time_in_calibration[input_src].cal = time_in_cal->cal;
		config_shaddow.time_in_calibration[input_src].offset_multiplier = time_in_cal->offset_multiplier;
	}
	return retval;
}

int Set_TimeInChannelId(u16 chid)
{
	int retval = 0;
	//configure the shaddow.
	config_shaddow.time_in_setting.channel_id = chid;
	retval += SetRegister(FPGA_IRIG_IN_CHANNEL_ID, chid);
	return retval;
}

int Get_TimeInChannelId(u16 * chid)
{
	int retval = 0;
	u32 val;
	retval = GetRegister(FPGA_IRIG_IN_CHANNEL_ID, &val);
	if(retval == 0)
	{
		*chid = val & 0xFFFF;
		config_shaddow.time_in_setting.channel_id = *chid;
	}
	return retval;
}

int Set_TimeInSource(vns_time_input_src_t src)
{
	int retval = 0;
	//configure the shaddow.
	retval += Stop_TimeInput();
	config_shaddow.time_in_setting.source = src;
	retval += SetRegister(FPGA_IRIG_IN_SRC, src);
	if(retval == 0)
	{
		switch(src)
		{
                    case (TIME_INPUT_SRC_IRIG_B):
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_IRIG_B]);
                        //Set_IEEE_1588_Mode(IEEE_1588_MASTER);
                        break;
                    case (TIME_INPUT_SRC_IRIG_A):
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_IRIG_A]);
                        //Set_IEEE_1588_Mode(IEEE_1588_MASTER);
                        break;
                    case (TIME_INPUT_SRC_IRIG_G):
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_IRIG_G]);
                        //Set_IEEE_1588_Mode(IEEE_1588_MASTER);
                        break;
                    case (TIME_INPUT_SRC_1588):
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_1588]);
                        Set_IEEE_1588_Mode(IEEE_1588_SLAVE);
                        break;
                    case (TIME_INPUT_SRC_1PPS):
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_1PPS]);
                        break;
                    case (TIME_INPUT_SRC_DISABLED):
                        //Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_DISABLED]);
                        break;
                    default://it must be one of the GPS settings
                        Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_GPS0]);
                        //Set_IEEE_1588_Mode(IEEE_1588_MASTER);
                        break;
		}
	}
	retval += Start_TimeInput();
	return retval;
}

int Get_TimeInSource(vns_time_input_src_t * src)
{
	int retval = 0;
	u32 val;
	retval = GetRegister(FPGA_IRIG_IN_SRC, &val);
	if(retval == 0)
	{
		*src = (vns_time_input_src_t)(val & (vns_reg_info[FPGA_IRIG_IN_SRC].mask >> vns_reg_info[FPGA_IRIG_IN_SRC].shift));
		config_shaddow.time_in_setting.source = *src;
	}
	return retval;
}

int Set_TimeInSignal(vns_time_input_signal_t sig)
{
	int retval = 0;
	//configure the shaddow.
	retval += Stop_TimeInput();
	config_shaddow.time_in_setting.signal = sig;
	retval += SetRegister(FPGA_IRIG_IN_SIGNAL, sig);
	retval += Start_TimeInput();
	return retval;
}

int Get_TimeInSignal(vns_time_input_signal_t * sig)
{
	int retval = 0;
	u32 val;
	retval = GetRegister(FPGA_IRIG_IN_SIGNAL, &val);
	if(retval == 0)
	{
		*sig = (vns_time_input_signal_t)(val & 0x3);
		config_shaddow.time_in_setting.signal = *sig;
	}
	return retval;
}

int Set_TimeInConfig(vns_time_input time_in)
{
	int retval = 0;
	vns_1588_type mode;
	//set the new config.
	retval += Stop_TimeInput();

	retval += Set_TimeInChannelId(time_in.channel_id);
	retval += Set_TimeInSource(time_in.source );
	retval += Get_IEEE_1588_Mode(&mode);

#if defined (BOARD_VNS_12_REF) || !HIDE_IRIG_INPUTS
	  /* IRIG VNS */
	retval += Set_TimeInSignal(time_in.signal );
	if(!retval ) {
            if(time_in.source == TIME_INPUT_SRC_1588 ) { /* 1588 must be a slave if TIMIN is 1588 */
                Set_IEEE_1588_Mode(IEEE_1588_SLAVE);
            }
            else if(mode == IEEE_1588_SLAVE) { /* You do not want 1588 to drive if another time input is selected */
                Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
            }
	}
#elif HIDE_IRIG_INPUTS
	retval += Set_TimeInSignal(time_in.signal );
	if(!retval ) {
            if(time_in.source == TIME_INPUT_SRC_1588 ) { /* 1588 must be a slave if TIMIN is 1588 */
                Set_IEEE_1588_Mode(IEEE_1588_SLAVE);
            }
            else { /* You do not want 1588 to drive if another time input is selected */
                Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
            }
        }
#else

	if(!retval ) {
            if(mode == IEEE_1588_SLAVE) {
                Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
            }
        }
        retval += Set_TimeInSignal(TIME_INPUT_TYPE_DC );
#endif

	retval += Start_TimeInput();
	return retval;
}

int Get_TimeInConfig(vns_time_input * time_in)
{
	int retval = 0;
	u16 chid;
	vns_time_input_src_t src;
	vns_time_input_signal_t sig;
	retval += Get_TimeInChannelId(&chid);
	if(retval == 0)
	{
		time_in->channel_id = chid;
	}
	retval += Get_TimeInSource(&src);
	if(retval == 0)
	{
		time_in->source = src;
	}
	retval += Get_TimeInSignal(&sig);
	if(retval == 0)
	{
		time_in->signal = sig;
	}
	return retval;
}

int Set_TimeOutCalibration(vns_time_out_cal time_out_cal)
{
	vns_time_output_t output_code;
	int retval = 0;
	switch(config_shaddow.time_out_setting.timecode)
	{
	case (TIME_OUTPUT_TYPE_IRIG_B):
		output_code = TIME_OUTPUT_TYPE_IRIG_B;
		break;
	case (TIME_OUTPUT_TYPE_IRIG_A):
		output_code = TIME_OUTPUT_TYPE_IRIG_A;
		break;
	case (TIME_OUTPUT_TYPE_IRIG_G):
		output_code = TIME_OUTPUT_TYPE_IRIG_G;
		break;
	default:
		output_code = TIME_OUTPUT_TYPE_DISABLED;
		break;
	}

	if(output_code != TIME_OUTPUT_TYPE_DISABLED)
	{

		//configure the shaddow.
		config_shaddow.time_out_calibration[output_code].coarse_cal = time_out_cal.coarse_cal;
		config_shaddow.time_out_calibration[output_code].fine_cal = time_out_cal.fine_cal;
		config_shaddow.time_out_calibration[output_code].med_cal = time_out_cal.med_cal;
		//set the new config.
		retval += SetRegister(FPGA_IRIG_OUT_TCG_CAL_COARSE, time_out_cal.coarse_cal);
		retval += SetRegister(FPGA_IRIG_OUT_TCG_CAL_MID, time_out_cal.med_cal);
		retval += SetRegister(FPGA_IRIG_OUT_TCG_CAL_FINE_MSB, (time_out_cal.fine_cal >> 8));
		retval += SetRegister(FPGA_IRIG_OUT_TCG_CAL_FINE_LSB, (time_out_cal.fine_cal & 0xFF));
	}
	return retval;
}

int Get_TimeOutCalibration(vns_time_out_cal * time_out_cal)
{
	int retval = 0;
	u32 val;
	vns_time_output_t output_code;

	switch(config_shaddow.time_out_setting.timecode)
	{
	case (TIME_OUTPUT_TYPE_IRIG_B):
		output_code = TIME_OUTPUT_TYPE_IRIG_B;
		break;
	case (TIME_OUTPUT_TYPE_IRIG_A):
		output_code = TIME_OUTPUT_TYPE_IRIG_A;
		break;
	case (TIME_OUTPUT_TYPE_IRIG_G):
		output_code = TIME_OUTPUT_TYPE_IRIG_G;
		break;
	default:
		output_code = TIME_OUTPUT_TYPE_DISABLED;
		break;
	}

	if(output_code != TIME_OUTPUT_TYPE_DISABLED)
	{

		retval += GetRegister(FPGA_IRIG_OUT_TCG_CAL_COARSE, &val);
		if(retval == 0)
		{
			time_out_cal->coarse_cal = val;
		}
		retval += GetRegister(FPGA_IRIG_OUT_TCG_CAL_MID, &val);
		if(retval == 0)
		{
			time_out_cal->med_cal = val;
		}
		retval += GetRegister(FPGA_IRIG_OUT_TCG_CAL_FINE_MSB, &val);
		if(retval == 0)
		{
			time_out_cal->fine_cal = val << 8;
		}
		retval += GetRegister(FPGA_IRIG_OUT_TCG_CAL_FINE_LSB, &val);
		if(retval == 0)
		{
			time_out_cal->fine_cal |= val;
		}

		if(retval == 0)
		{
			//make sure the shaddow is the same
			config_shaddow.time_out_calibration[output_code].coarse_cal = time_out_cal->coarse_cal;
			config_shaddow.time_out_calibration[output_code].fine_cal = time_out_cal->fine_cal;
			config_shaddow.time_out_calibration[output_code].med_cal = time_out_cal->med_cal;
		}
	}

	return retval;
}

int Set_TimeOutChannelId(u16 chid)
{
	int retval = 0;
	//configure the shaddow.
	config_shaddow.time_out_setting.channel_id = chid;
	retval += SetRegister(FPGA_IRIG_OUT_CHANNEL_ID, chid);
	return retval;
}

int Get_TimeOutChannelId(u16 * chid)
{
	int retval = 0;
	u32 val;
	retval = GetRegister(FPGA_IRIG_OUT_CHANNEL_ID, &val);
	if(retval == 0)
	{
		*chid = val & 0xFFFF;
		config_shaddow.time_out_setting.channel_id = *chid;
	}
	return retval;
}

int Set_TimeOutTimeCode(vns_time_output_t tc)
{
	int retval = 0;
	//retval += Stop_TimeOutput();
	retval += SetRegister(FPGA_IRIG_OUT_TIMECODE, tc);
	Set_TimeOutMode( TIME_OUTPUT_MODE_TCG);
	if (retval == 0)
	{
		config_shaddow.time_out_setting.timecode = tc;
		switch(tc)
		{
		case (TIME_OUTPUT_TYPE_IRIG_B):
			retval += Set_TimeOutCalibration(config_shaddow.time_out_calibration[TIME_OUTPUT_TYPE_IRIG_B]);
			break;
		case (TIME_OUTPUT_TYPE_IRIG_A):
			retval += Set_TimeOutCalibration(config_shaddow.time_out_calibration[TIME_OUTPUT_TYPE_IRIG_A]);
			break;
		case (TIME_OUTPUT_TYPE_IRIG_G):
			retval += Set_TimeOutCalibration(config_shaddow.time_out_calibration[TIME_OUTPUT_TYPE_IRIG_G]);
			break;
		default:
			//output_code = TIME_OUTPUT_DISABLED;
			break;
		}
	}
	//retval += Start_TimeOutput();
	return retval;
}

int Get_TimeOutTimeCode(vns_time_output_t *tc)
{
	int retval = 0;
	u32 val;
	retval += GetRegister(FPGA_IRIG_OUT_TIMECODE, &val);
	if(retval == 0)
	{
		*tc = (vns_time_output_t)(val & 0x3);
	}
	if(retval == 0)
	{
		//make sure the shaddow matches...
		config_shaddow.time_out_setting.timecode = *tc;
	}
	return retval;
}

int Set_TimeOutMode(vns_time_output_mode_t mode)
{
	int retval = 0;
	//retval += Stop_TimeOutput();

	retval += SetRegister(FPGA_IRIG_OUT_MODE, mode);
	if(retval == 0)
	{
		config_shaddow.time_out_setting.mode = mode;
	}
	//retval += Start_TimeOutput();
	return retval;
}

int Get_TimeOutMode(vns_time_output_mode_t *mode)
{
	int retval = 0;
	u32 val;
	retval += GetRegister(FPGA_IRIG_OUT_MODE, &val);
	if(retval == 0)
	{
		*mode = (vns_time_output_mode_t)(val & 0x3);
	}
	if(retval == 0)
	{
		//make sure the shaddow matches...
		config_shaddow.time_out_setting.mode = *mode;
	}
	return retval;
}

int Set_Ttl1ppsMode(BOOL enable)
{
	int retval = 0;
	//retval += Stop_TimeOutput();
	u32 mode = (enable ? 1 : 0);
	retval += SetRegister(FPGA_TTL_1PPS_ENABLE, mode);
	if(retval == 0)
	{
		config_shaddow.time_out_setting.irig_dc_1pps_mode = (mode ? TRUE : FALSE);
	}
	return retval;
}

int Get_Ttl1ppsMode(BOOL *enable)
{
	int retval = 0;
	u32 val;
	retval += GetRegister(FPGA_TTL_1PPS_ENABLE, &val);
	if(retval == 0)
	{
		*enable = (val ? TRUE : FALSE);
	}
	if(retval == 0)
	{
		//make sure the shaddow matches...
		config_shaddow.time_out_setting.irig_dc_1pps_mode = *enable;
	}
	return retval;
}

//	Note, this also sets up or changes ptp clock instance 0...
// 	Created a timer to perform init function init_setup_timer ("do not call this during init.")
int Set_IEEE_1588_Mode(vns_1588_type mode)
{
    return Set_IEEE_1588_Mode_Mod_clk( mode, FALSE);
}
int Set_IEEE_1588_Mode_Mod_clk(vns_1588_type mode, BOOL modify_clock)
{
	int retval = 0;
	u32 cur_mode;
        if(modify_clock)
            T_D("modify_clock ");

	retval = GetRegister(FPGA_IEEE_1588_MODE, &cur_mode);
        T_D("Set_IEEE_1588_Mode_Mod_clk mode = %d", mode);
        T_D("Set_IEEE_1588_Mode_Mod_clk cur_mode = %d", cur_mode);
	if((cur_mode != 2) && (cur_mode != 3) && (cur_mode != 0))
	{
		T_E("1588 Current mode is invalid...");
	}
	if((cur_mode != (u32)mode) && (retval == 0))
	{
            switch(mode)
            {
                case IEEE_1588_SLAVE:
                    T_D("Setting 1588 to Slave Mode");
                    retval += SetRegister(FPGA_IEEE_1588_MODE, (u32)mode);
                    break;
                case IEEE_1588_MASTER:
                    T_D("Setting 1588 to Master Mode");
                    retval += SetRegister(FPGA_IEEE_1588_MODE, (u32)mode);
                    break;
                default:
                    /* cancel_timer(); */
                    T_D("Setting 1588 to Disabled");
                    retval += SetRegister(FPGA_IEEE_1588_MODE, (u32)mode);
                    break;
            }
            if(retval == 0)
                config_shaddow.time_out_setting.ieee_1588_type = mode;
	}
	if(modify_clock)
	{
            switch(mode)
            {
                case IEEE_1588_SLAVE:
                    create_ptp_clock(FALSE);
                    break;
                case IEEE_1588_MASTER:
                    create_ptp_clock(TRUE);
                    break;
                default:
                    delete_ptp_clock();
                    break;
            }
	}
	return retval;
}

int Get_IEEE_1588_Mode(vns_1588_type * mode)
{
	int retval = 0;
	u32 val;
	retval += GetRegister(FPGA_IEEE_1588_MODE, &val);
	if(retval == 0)
	{
		if((val != 2) && (val != 3) && (val != 0))
		{
			T_E("1588 Current mode is invalid...");
			*mode = IEEE_1588_DISABLED;
		}
		else
		{
			*mode = (vns_1588_type)val;
		}
	}
	return retval;
}

int Set_TimeOutConfig(vns_time_output time_out)
{
	int retval = 0;
	//set the new config.
	retval += Stop_TimeOutput();

	retval += Set_TimeOutChannelId(time_out.channel_id);
	retval += Set_TimeOutTimeCode(time_out.timecode);
	retval += Set_IEEE_1588_Mode(time_out.ieee_1588_type);
	retval += SetRegister(FPGA_IEEE_1588_MODE, time_out.ieee_1588_type);
	retval += Set_TimeOutMode(time_out.mode);
	retval += Set_Ttl1ppsMode(time_out.irig_dc_1pps_mode);
	retval += Set_GPSWrittenLeapValue(time_out.leap_seconds);

	retval += Start_TimeOutput();
	return retval;
}

int Get_TimeOutConfig(vns_time_output * time_out)
{
	int retval = 0;
	u16 chid;
	vns_time_output_t tc;
	vns_time_output_mode_t mode;
	vns_1588_type ieee_1588_mode;

	retval += Get_TimeOutChannelId(&chid);
	if(retval == 0)
	{
		time_out->channel_id = chid;
	}
	retval += Get_TimeOutTimeCode(&tc);
	if(retval == 0)
	{
		time_out->timecode = tc;
	}
	retval += Get_TimeOutMode(&mode);
	if(retval == 0)
	{
		time_out->mode = mode;
	}
	retval += Get_IEEE_1588_Mode(&ieee_1588_mode);
	if(retval == 0)
	{
		time_out->ieee_1588_type = ieee_1588_mode;
	}
	return retval;
}

int Start_TimeInput()
{
	return SetRegister(FPGA_IRIG_IN_ENABLE, 1);
}

int Stop_TimeInput()
{
	return SetRegister(FPGA_IRIG_IN_ENABLE, 0);
}

int Start_TimeOutput()
{
	return SetRegister(FPGA_IRIG_OUT_ENABLE, 1);
}

int Stop_TimeOutput()
{
	return SetRegister(FPGA_IRIG_OUT_ENABLE, 0);
}

vns_status_t GetStatus()
{
	return vns_status;
}
void FPGA_Debug_disable_timer(BOOL test_mode_on)
{
    vns_global.fpga_update.disable_timer = test_mode_on;
    if(test_mode_on)
        T_D("Timer off");
    else
        T_D("Timer on");
}

void FPGA_Debug_Test_Mode(BOOL test_mode_on)
{
	debug_test_mode = test_mode_on;
}
int Set_discretes_out(u32 value)
{
    u32 do_state = 0;
    //	u32 new_do_state = 0;
    u32 mask = 0;
    int i;
    if(Get_DiscreteOutputState(&do_state))//int Get_DiscreteOutputState(u32 * do_state)
    {
        return -1;
    }
    for(i = 0; i < DISCRETE_OUT_MAX; i++)
    {
        if(config_shaddow.do_config.do_setting[i] == DO_SETTING_USER)
        {
            mask |= (1 << i);
        }
    }
    do_state &=  (~mask);
    do_state |= (mask & (~value));


    if(Set_DiscreteOutputState(do_state))//int Get_DiscreteOutputState(u32 * do_state)
    {
        return -1;
    }


    return 0;



}
BOOL is_mode2_led_control()
{
    if(vns_global.board_conf.ies_board[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250204) 
        return TRUE;
    if( vns_global.board_conf.ies_board[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250004 &&
         vns_global.board_conf.ies_board[FLASH_CONF_INTERCONNECT_REV_BRD_TAG] > 4 ) 
        return TRUE;
    return FALSE;
}
static int _set_led_discrete_registers(u32 led_red_blue, u32 led_green, u32 led_flashing, u32 discrete_out)
{
    int retval = 0;
    vns_registers_t r_register,
                    g_register,
                    f_register;
    if( vns_global.fw_major >= 3 && vns_global.fw_minor >= 2 ) {
    /* if new board */
        r_register = FPGA_LED_CONTROL2_R;
        g_register  = FPGA_LED_CONTROL2_G;
        f_register  = FPGA_LED_CONTROL2_FLASH;

#if defined (BOARD_VNS_12_REF )
        if(is_mode2_led_control()) 
        {
            /* setting mode 1 for new interconnect board */
            retval += SetRegister(FPGA_LED_CONTROL2_MODE, TRUE);
        }
        else 
        {
            retval += SetRegister(FPGA_LED_CONTROL2_MODE, FALSE);
        }
#endif
                    
    }
    else {
    /* if old board */
        r_register = FPGA_LED_CONTROL_R_B;
        g_register  = FPGA_LED_CONTROL_G;
        f_register  = FPGA_LED_CONTROL_FLASH;
    }

    retval += SetRegister(r_register,led_red_blue);
    retval += SetRegister(g_register, led_green);
    retval += SetRegister(f_register, led_flashing);

    //write discrete info to the fpga
    retval += SetRegister(FPGA_DISC_OUT_STATE, ~discrete_out);

    return retval;
}
static void ies_8_update_time_lock_status(vtss_timer_t *timer)
{
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    /* Shared for both */
    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Update PTP status.");

    if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
        //failed
        //T_W("VNS_FGA: Failed to get ptp status... Attempting to resolve this...");
        if(timer->period_us != 1000000)
        {
            timer->period_us = 1000000;
            T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
        }
        ptp_lock = FALSE;
    }else {
        if(timer->period_us != 500000) {
            timer->period_us = 500000; //change to 1/10 second interval
        }
        ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);
        if( ptp_lock )
            T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "PTP locked");
        else
            T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "PTP NOT locked.");
    }

}
static int _setledsanddiscretes(u32 link, u32 activity, u32 speed)
{
    int retval = 0;
    int i;
    u32 time_in_health;
    u32 time_out_health;
    vns_time_input_src_t time_source;
    u32 ledg = 0;
    u32 ledr_b = 0;
    u32 led_flash = 0;

    u32 disc_out = 0;
    u32 do_state = 0;

    //setup port leds
    ledg = link;
    ledr_b = link & speed;
    led_flash = link & activity;

    time_source = (vns_time_input_src_t)config_shaddow.time_in_setting.source;
    if(GetTimeInHealth( &time_in_health))
    {
        time_in_health = HEALTH_SETUP_FAIL;
        vns_status = VNS_STATE_FAIL;
    }
    if(GetTimeOutHealth( &time_out_health))
    {
        time_out_health = HEALTH_SETUP_FAIL;
        vns_status = VNS_STATE_FAIL;
    }

    //setup status led
    if((vns_status == VNS_STATE_FAIL) || (vns_status == VNS_STATE_ERROR))
    {

        ledr_b |= STATUS_LED_RG;
    }
    else
    {
        ledg |= STATUS_LED_RG;
    }

    //setup time and gps leds
    switch(time_source)
    {
        case TIME_INPUT_SRC_IRIG_B:
        case TIME_INPUT_SRC_IRIG_A:
        case TIME_INPUT_SRC_IRIG_G:
        case TIME_INPUT_SRC_1PPS:
            if((time_in_health == 0))
            {
                //set to yellow
                ledg 	|= TIME_LOCK_LED_RG;
                ledr_b 	|= TIME_LOCK_LED_RG;

            }
            else
            {//set to red
                ledr_b 	|= TIME_LOCK_LED_RG;
            }
            //turn off gps led
            ledg 	&= ~GPS_LED_RG;
            ledr_b 	&= ~GPS_LED_B;
            break;
        case TIME_INPUT_SRC_1588:
            if(ptp_lock)
                ledg 	|= TIME_LOCK_LED_RG;
            else
                ledr_b |= TIME_LOCK_LED_RG;
            //turn off gps led
            ledg 	&= ~GPS_LED_RG;
            ledr_b 	&= ~GPS_LED_B;
            break;
        case TIME_INPUT_SRC_GPS0:
        case TIME_INPUT_SRC_GPS3:
        case TIME_INPUT_SRC_GPS5:
            if((time_in_health == 0))
            {
                //set time led to yellow
                ledg 	|= TIME_LOCK_LED_RG;
                ledr_b 	|= TIME_LOCK_LED_RG;
                //set gps led to green
                ledg 	|= GPS_LED_RG;
            }
            else
            {//set to red
                ledr_b 	|= TIME_LOCK_LED_RG;
                ledr_b 	|= GPS_LED_RG;

            }
            break;
        case TIME_INPUT_SRC_DISABLED:
        default:
            /* Turn off LED's when disabled per engineering meeting */
            ledg 	&= ~TIME_LOCK_LED_RG;
            ledr_b 	&= ~TIME_LOCK_LED_RG;
            led_flash &= ~TIME_LOCK_LED_RG;
            //time input disabled flash yellow
            //set gps led to yellow
            /* ledg 	|= GPS_LED_RG; */
            /* ledr_b 	|= GPS_LED_RG; */
            /* led_flash |= GPS_LED_RG; */
            break;
    }

#if defined (BOARD_VNS_8_REF)
    if( ptp_lock ) 
        ledr_b |= TIME_LOCK_LED_B;
    else
        ledr_b 	&= ~TIME_LOCK_LED_B;
    
    ledg 	&= ~TIME_LOCK_LED_RG;
    led_flash &= ~TIME_LOCK_LED_RG;
#endif
    //setup user led 1 (blank)

    //setup user led 2 (this is GPS)

    //setup discrete output 1 - 12

    //
    if(Get_DiscreteOutputState(&do_state))
    {
        disc_out |= DISC_OUT_FAULT;
    }

    for(i = 0; i < DISCRETE_OUT_MAX; i++)
    {

        switch(config_shaddow.do_config.do_setting[i])
        {
            case 	DO_SETTING_P1_ACTIVITY:
                if(activity & PORT_1)
                {
                    disc_out |= (1 << i);
                }
                break;
            case 	DO_SETTING_P2_ACTIVITY:
                if(activity & PORT_2)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P3_ACTIVITY:
                if(activity & PORT_3)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P4_ACTIVITY:
                if(activity & PORT_4)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P5_ACTIVITY:
                if(activity & PORT_5)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P6_ACTIVITY:
                if(activity & PORT_6)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P7_ACTIVITY:
                if(activity & PORT_7)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P8_ACTIVITY:
                if(activity & PORT_8)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P9_ACTIVITY:
                if(activity & PORT_9)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P10_ACTIVITY:
                if(activity & PORT_10)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P11_ACTIVITY:
                if(activity & PORT_11)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P12_ACTIVITY:
                if(activity & PORT_12)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P1_LINK:
                if(link & PORT_1)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P2_LINK:
                if(link & PORT_2)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P3_LINK:
                if(link & PORT_3)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P4_LINK:
                if(link & PORT_4)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P5_LINK:
                if(link & PORT_5)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P6_LINK:
                if(link & PORT_6)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P7_LINK:
                if(link & PORT_7)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P8_LINK:
                if(link & PORT_8)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P9_LINK:
                if(link & PORT_9)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P10_LINK:
                if(link & PORT_10)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P11_LINK:
                if(link & PORT_11)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_P12_LINK:
                if(link & PORT_12)
                {
                    disc_out |= (1 << i);
                }
                break;
            case	DO_SETTING_TIME_LOCK:
                if(0 == time_in_health)
                {
                    disc_out |= (1 << i);
                }
                else
                {
                    disc_out &= ~(1 << i);
                }
                break;
            case	DO_SETTING_1588_LOCK:
                if(time_source == TIME_INPUT_SRC_1588)
                {
                    if(ptp_lock == TRUE )
                        disc_out |= (1 << i);
                    else
                        disc_out &= ~(1 << i);
                }
                break;
            case	DO_SETTING_GPS_LOCK:
                if(((time_source == TIME_INPUT_SRC_GPS0) ||(time_source == TIME_INPUT_SRC_GPS3)||(time_source == TIME_INPUT_SRC_GPS5)))
                {
                    if(time_in_health == 0)
                    {
                        disc_out |= (1 << i);
                    }
                    else
                    {
                        disc_out &= ~(1 << i);
                    }
                }
                break;
            case	DO_SETTING_SWITCH_ERROR:
                break;
            case	DO_SETTING_FPGA_ERROR:
                break;
            case	DO_SETTING_USER:

                if(((~do_state) & (1 << i)) != 0)
                {
                    disc_out |= (1 << i);
                }
                break;
            default: //		DO_SETTING_DISABLED:
                break;

        }
    }
    if((vns_status == VNS_STATE_FAIL) || (vns_status == VNS_STATE_ERROR))
    {
        disc_out |= DISC_OUT_FAULT;
    }
    //write led info it to the fpga

    if(!debug_test_mode)
    {
        _set_led_discrete_registers(ledr_b, ledg, led_flash, disc_out);
    }

    return retval;
}
void reset_leds_and_discretes(void)
{
    SetRegister(FPGA_LED_CONTROL_R_B, 0x00);
    SetRegister(FPGA_LED_CONTROL_G, 0x00);
    SetRegister(FPGA_LED_CONTROL_FLASH, 0x00);

    //write discrete info to the fpga
    SetRegister(FPGA_DISC_OUT_STATE, 0x00);
}

int adjust_leap_seconds(void)
{
    u16 recieved_leap_seconds = 0,  
        stored_leap_seconds = 0;
    int retval = 0;

    retval  = Get_GPSRecievedLeapValue(&recieved_leap_seconds);
    if(retval)
        T_D("Get_GPSRecievedLeapValue Failed: 0x%X", retval);
    /* T_D("recieved_leap_seconds : 0x%x", recieved_leap_seconds); */
    retval += Get_StoredLeapSeconds(&stored_leap_seconds);
    if(retval)
        T_D("Get_StoredLeapSeconds Failed: 0x%X", retval);
    /* T_D("stored_leap_seconds : 0x%X", stored_leap_seconds); */
    if( recieved_leap_seconds > stored_leap_seconds)
    {
        Set_StoredLeapSeconds(recieved_leap_seconds);
        Set_GPSWrittenLeapValue(recieved_leap_seconds);
    }

    return retval;
}

int Set_leds_and_discretes(u32 link, u32 activity, u32 speed)
{
    vns_global.link     = link;
    vns_global.activity = activity;
    vns_global.speed    = speed;

    if(vns_global.init_complete && (!vns_global.fpga_update.disable_timer))
    {
        return _setledsanddiscretes(link, activity, speed);
    }
    else
    {
        return 0;
    }

}

int set_vns_config(void)
{
	int retval = 0;
	if(config_shaddow.version != VNS_CONFIG_VERSION)
	{
		T_E("...VNS CONFIG SHOULD BE SET, BUT IT IS NOT...\n");
	}
	if(Set_DiscreteInputConfig(config_shaddow.di_config)){
		retval += 1;
		T_E("VNS FPGA: Failed to set discrete input config!\n");
	}
	if(Set_DiscreteOutputConfig(config_shaddow.do_config)){
		retval += 1;
		T_E("VNS FPGA: Failed to set discrete output config!\n");
	}
	if(Set_TimeInConfig(config_shaddow.time_in_setting)){
		retval += 1;
		T_E("VNS FPGA: Failed to set time input config!\n");
	}
	if(config_shaddow.time_in_setting.source < TIME_INPUT_SRC_GPS3)
	{
		if(Set_TimeInCalibration(config_shaddow.time_in_calibration[config_shaddow.time_in_setting.source]))
		{
			retval += 1;
			T_E("VNS FPGA: Failed to set time input calibration!\n");
		}
	}
	else
	{
		if(Set_TimeInCalibration(config_shaddow.time_in_calibration[TIME_INPUT_SRC_GPS0]))
		{
			retval += 1;
			T_E("VNS FPGA: Failed to set time input calibration!\n");
		}
	}
	if(Set_TimeOutConfig(config_shaddow.time_out_setting)){
		retval += 1;
		T_E("VNS FPGA: Failed to set time output config!\n");
	}
	if(config_shaddow.time_out_setting.timecode <= TIME_OUTPUT_TYPE_DISABLED)
	{
		if(Set_TimeOutCalibration(config_shaddow.time_out_calibration[config_shaddow.time_out_setting.timecode]))
		{
			retval += 1;
			T_E("VNS FPGA: Failed to set time output calibration!\n");
		}
	}
	else
	{
		retval += 1;
		T_E("VNS FPGA: Failed to set time output calibration!\n");
	}
        T_D("epe_conf->mode %u", config_shaddow.epe_config.mode );
        T_D("epe_conf->bit_rate %u", config_shaddow.epe_config.bit_rate );
        T_D("epe_conf->ch7_data_word_count %u", config_shaddow.epe_config.ch7_data_word_count );
        if(!set_vns_fpga_epe_conf(config_shaddow.epe_config))
        {
            retval += 1;
            T_E("VNS FPGA: Failed to set Ethernet PCM Encoder!\n");
        }
	return retval;
}

static void vns_fpga_conf_read(vtss_isid_t isid_add, BOOL force_default)
{
    vns_fpga_conf_t * vns_cfg_blk = NULL;
    vns_fpga_conf_t vns_cfg;
    u32 size;
    BOOL do_create = 0;
    int i;
    /* Open or create VNS configuration block */
    if ((vns_cfg_blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, &size)) == NULL ||
            size != sizeof(*vns_cfg_blk)) {
        T_W("VNS_CONFIG: conf_sec_open failed or size mismatch, creating defaults");

        vns_cfg_blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, sizeof(*vns_cfg_blk));
        do_create = 1;
    } else if (vns_cfg_blk->version != VNS_CONFIG_VERSION) {
        T_W("VNS_CONFIG: Version mismatch, creating defaults");
        do_create = 1;
    }

    if (force_default || do_create)
    {
        if(vns_cfg_blk == NULL)
            vns_cfg_blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF,sizeof(*vns_cfg_blk)) ;
        if(vns_cfg_blk != NULL)
        {
            T_D("VNS CONFIG: Setting config to default, isid:%d",isid_add);
            /* Use start values */
            vns_cfg_blk->version = VNS_CONFIG_VERSION;
            vns_cfg_blk->verbose = TRUE;
            for(i = 0; i < DISCRETE_IN_MAX; i++)
            {
                vns_cfg_blk->di_config.di_setting[i] = DI_SETTING_DISABLED;
            }
            for(i = 0; i < DISCRETE_OUT_MAX; i++)
            {
                vns_cfg_blk->do_config.do_setting[i] = (vns_do_setting_t)(i);
            }

            vns_cfg_blk->time_in_setting.channel_id         = 1;
#if defined (BOARD_VNS_12_REF)
            vns_cfg_blk->time_in_setting.signal             = TIME_INPUT_TYPE_DC;
            vns_cfg_blk->time_in_setting.source             = TIME_INPUT_SRC_GPS0;
#else
            vns_cfg_blk->time_in_setting.signal             = TIME_INPUT_TYPE_DC;
            /* time_in_setting.source has to be initially disabled otherwise PTP clock is not created */
            vns_cfg_blk->time_in_setting.source             = TIME_INPUT_SRC_DISABLED;
#endif
            vns_cfg_blk->time_out_setting.channel_id        = 1;
            vns_cfg_blk->time_out_setting.mode              = TIME_OUTPUT_MODE_TCG;
            vns_cfg_blk->time_out_setting.ieee_1588_type    = IEEE_1588_DISABLED;
            vns_cfg_blk->time_out_setting.timecode          = TIME_OUTPUT_TYPE_IRIG_B;
            vns_cfg_blk->time_out_setting.irig_dc_1pps_mode = FALSE;
            vns_cfg_blk->time_out_setting.leap_seconds      = 0;

            /* DEFAULT EPE CONFIGURATION */
            vns_cfg_blk->epe_config.mode                    = VNS_EPE_DISABLE;
            vns_cfg_blk->epe_config.bit_rate                = VNS_EPE_BIT_RATE_MAX;
            vns_cfg_blk->epe_config.ch7_data_word_count     = CH7_FRAME_SIZE_DEFAULT;


            //DEFAULT IRIG-B INPUT CALIBRATION

            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_B].offset_multiplier = 0x21;
            //DEFAULT IRIG-A INPUT CALIBRATION

            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_A].offset_multiplier = 0x21;
            //DEFAULT IRIG-G INPUT CALIBRATION

            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_G].offset_multiplier = 0x21;
            //DEFAULT IEEE-1588 INPUT CALIBRATION

            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_1PPS].cal = 0x00;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_1PPS].offset_multiplier = 0x21;
            //DEFAULT 1PPS INPUT CALIBRATION
            
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_1588].cal = 0x00;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_1588].offset_multiplier = 0x21;
            //DEFAULT GPS INPUT CALIBRATION
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_GPS0].cal = 0x00;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_GPS0].offset_multiplier = 0x21;

            //DEFAULT IRIG-B OUTPUT CALIBRATION
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_B].coarse_cal = 0x02;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_B].med_cal = 0x00;

            //DEFAULT IRIG-A OUTPUT CALIBRATION
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_A].coarse_cal = 0x02;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_A].med_cal = 0x00;

            //DEFAULT IRIG-G OUTPUT CALIBRATION
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_G].coarse_cal = 0x02;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_G].med_cal = 0x00;

#if defined (BOARD_VNS_6_REF) || defined (BOARD_VNS_8_REF)
            //DEFAULT IRIG INPUT CALIBRATION
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_B].cal = 0x1388;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_A].cal = 0x650;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_G].cal = 0x90;

            //DEFAULT IRIG OUTPUT CALIBRATION
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_B].fine_cal = 0x18;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_A].fine_cal = 0x18;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_G].fine_cal = 0x18;

#else
            //DEFAULT IRIG INPUT CALIBRATION
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_B].cal = 0x2790;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_A].cal = 0x3DA;
            vns_cfg_blk->time_in_calibration[TIME_INPUT_SRC_IRIG_G].cal = 0x54;
            //DEFAULT IRIG OUTPUT CALIBRATION
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_B].fine_cal = 0x17;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_A].fine_cal = 0x17;
            vns_cfg_blk->time_out_calibration[TIME_OUTPUT_TYPE_IRIG_G].fine_cal = 0x17;
#endif
        }
    }

    if (vns_cfg_blk != NULL) {
        /* restore from flash  */
        T_D("VNS CONFIG: Getting stored VNS Config Settings, isid:%d",isid_add);
        /* Move entries from flash to local structure and chip */
        vns_cfg.version = vns_cfg_blk->version;
        for(i = 0; i < DISCRETE_IN_MAX; i++)
        {
            vns_cfg.di_config.di_setting[i] = vns_cfg_blk->di_config.di_setting[i];
        }
        for(i = 0; i < DISCRETE_OUT_MAX; i++)
        {
            vns_cfg.do_config.do_setting[i] = vns_cfg_blk->do_config.do_setting[i];
        }
        vns_cfg.time_in_setting.channel_id         = vns_cfg_blk->time_in_setting.channel_id;
        vns_cfg.time_in_setting.signal             = vns_cfg_blk->time_in_setting.signal;
        vns_cfg.time_in_setting.source             = vns_cfg_blk->time_in_setting.source;
        vns_cfg.time_out_setting.channel_id        = vns_cfg_blk->time_out_setting.channel_id;
        vns_cfg.time_out_setting.mode              = vns_cfg_blk->time_out_setting.mode;
        vns_cfg.time_out_setting.ieee_1588_type    = vns_cfg_blk->time_out_setting.ieee_1588_type;
        T_D("vns_cfg: ieee_1588_type = %d",vns_cfg.time_out_setting.ieee_1588_type );
        T_D("vns_cfg_blk: ieee_1588_type = %d",vns_cfg_blk->time_out_setting.ieee_1588_type);
        vns_cfg.time_out_setting.timecode          = vns_cfg_blk->time_out_setting.timecode;
        vns_cfg.time_out_setting.irig_dc_1pps_mode = vns_cfg_blk->time_out_setting.irig_dc_1pps_mode;
        vns_cfg.time_out_setting.leap_seconds      = vns_cfg_blk->time_out_setting.leap_seconds;

        /* EPE CONFIGURATION */
        vns_cfg.epe_config = vns_cfg_blk->epe_config;
        /* vns_cfg.epe_config.mode                       = vns_cfg_blk->epe_config.mode; */
        /* vns_cfg.epe_config.bit_rate                   = vns_cfg_blk->epe_config.bit_rate; */
        /* vns_cfg.epe_config.ch7_data_word_count        = vns_cfg_blk->epe_config.ch7_data_word_count; */
        T_D("epe_conf->mode %u", vns_cfg_blk->epe_config.mode );
        T_D("epe_conf->bit_rate %u", vns_cfg_blk->epe_config.bit_rate );
        T_D("epe_conf->ch7_data_word_count %u", vns_cfg_blk->epe_config.ch7_data_word_count );

        for(i = 0; i < TIME_IN_CAL_TABLE_ENTRIES; i++)
        {
            vns_cfg.time_in_calibration[i].cal = vns_cfg_blk->time_in_calibration[i].cal;
            //vns_cfg.time_in_calibration[i].small_cal = vns_cfg_blk->time_in_calibration[i].small_cal;
            //vns_cfg.time_in_calibration[i].rtc_cal = vns_cfg_blk->time_in_calibration[i].rtc_cal;
            vns_cfg.time_in_calibration[i].offset_multiplier = vns_cfg_blk->time_in_calibration[i].offset_multiplier;
        }
        for(i = 0; i < TIME_OUT_CAL_TABLE_ENTRIES; i++)
        {
            vns_cfg.time_out_calibration[i].coarse_cal = vns_cfg_blk->time_out_calibration[i].coarse_cal;
            vns_cfg.time_out_calibration[i].med_cal = vns_cfg_blk->time_out_calibration[i].med_cal;
            vns_cfg.time_out_calibration[i].fine_cal = vns_cfg_blk->time_out_calibration[i].fine_cal;
        }
    }

    if (vns_cfg_blk == NULL) {
        T_E("Failed to open VNS config");
    } else {
        vns_cfg_blk->version = VNS_CONFIG_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF);
        set_config_shaddow(vns_cfg);

        //        if(set_vns_config())
        //        {
        //        	T_E("Failed to set VNS config");
        //        }
        //this should have already been done...
        if(force_default || do_create)
        {
            if(save_vns_config())
            {
                T_E("Failed to save default VNS config");
            }
        }
    }


}

int save_vns_config()//vns_fpga_conf_t config)
{
	vns_fpga_conf_t * vns_cfg_blk;
	u32 size;
	int i;
	/* Open or create VNS configuration block */
	vns_cfg_blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, &size);
	if ( (vns_cfg_blk == NULL) || (size != sizeof(*vns_cfg_blk)) ) {
		T_W("VNS_CONFIG: conf_sec_open failed or size mismatch, creating defaults");

		vns_cfg_blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF, sizeof(*vns_cfg_blk));
	}

	if (vns_cfg_blk != NULL) {
		T_D("VNS CONFIG: Writing config to flash, I think...");
		/* Use start values */
		vns_cfg_blk->version   = VNS_CONFIG_VERSION;
		vns_cfg_blk->verbose   = config_shaddow.verbose;
		vns_cfg_blk->reserved1 = 0;
		vns_cfg_blk->reserved2 = 0;

		vns_cfg_blk->time_in_setting.channel_id         = config_shaddow.time_in_setting.channel_id;
		vns_cfg_blk->time_in_setting.signal             = config_shaddow.time_in_setting.signal;
		vns_cfg_blk->time_in_setting.source             = config_shaddow.time_in_setting.source;
		vns_cfg_blk->time_out_setting.channel_id        = config_shaddow.time_out_setting.channel_id;
		vns_cfg_blk->time_out_setting.mode              = config_shaddow.time_out_setting.mode;
		vns_cfg_blk->time_out_setting.ieee_1588_type    = config_shaddow.time_out_setting.ieee_1588_type;
		T_D("config_shaddow ieee_1588_type =  %d", config_shaddow.time_out_setting.ieee_1588_type);
		T_D("vns_cfg_blk ieee_1588_type =  %d", vns_cfg_blk->time_out_setting.ieee_1588_type);
		vns_cfg_blk->time_out_setting.timecode          = config_shaddow.time_out_setting.timecode;
		vns_cfg_blk->time_out_setting.irig_dc_1pps_mode = config_shaddow.time_out_setting.irig_dc_1pps_mode;
		vns_cfg_blk->time_out_setting.reserved1         = 0;
		vns_cfg_blk->time_out_setting.leap_seconds      = config_shaddow.time_out_setting.leap_seconds;

            /* EPE CONFIGURATION */
                vns_cfg_blk->epe_config = config_shaddow.epe_config;
                /* vns_cfg_blk->epe_config.mode                       = config_shaddow.epe_config.mode; */
                /* vns_cfg_blk->epe_config.bit_rate                   = config_shaddow.epe_config.bit_rate; */
                /* vns_cfg_blk->epe_config.ch7_data_word_count        = config_shaddow.epe_config.ch7_data_word_count; */

                T_D("epe_conf->mode %u", config_shaddow.epe_config.mode );
                T_D("epe_conf->bit_rate %u", config_shaddow.epe_config.bit_rate );
                T_D("epe_conf->ch7_data_word_count %u", config_shaddow.epe_config.ch7_data_word_count );

		for(i = 0; i < TIME_IN_CAL_TABLE_ENTRIES; i++)
		{
			vns_cfg_blk->time_in_calibration[i].cal = config_shaddow.time_in_calibration[i].cal;
			vns_cfg_blk->time_in_calibration[i].offset_multiplier = config_shaddow.time_in_calibration[i].offset_multiplier;
		}
		for(i = 0; i < TIME_OUT_CAL_TABLE_ENTRIES; i++)
		{
			vns_cfg_blk->time_out_calibration[i].coarse_cal = config_shaddow.time_out_calibration[i].coarse_cal;
			vns_cfg_blk->time_out_calibration[i].med_cal = config_shaddow.time_out_calibration[i].med_cal;
			vns_cfg_blk->time_out_calibration[i].fine_cal = config_shaddow.time_out_calibration[i].fine_cal;
			vns_cfg_blk->time_out_calibration[i].reserved1 = 0;
		}
		for(i = 0; i < DISCRETE_IN_MAX; i++)
		{
			vns_cfg_blk->di_config.di_setting[i] = config_shaddow.di_config.di_setting[i];
		}
		for(i = 0; i < DISCRETE_OUT_MAX; i++)
		{
			vns_cfg_blk->do_config.do_setting[i] = config_shaddow.do_config.do_setting[i];
		}
		T_D("Before conf_sec_close");
		conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_VNS_FPGA_CONF);
		T_D("After conf_sec_close");
	}
	else {
		T_E("VNS_CONFIG: conf_sec_open failed could not save VNS Configuration;");
		return 1;
	}
	return 0;
}

// todo: add these parameters to the config and set them when in slave mode...
int set_ptp_slave_cfg(u32 stable_offset, u32 offset_ok, u32 offset_fail)
{
    ptp_clock_slave_cfg_t slave_cfg;
    slave_cfg.stable_offset = stable_offset;
    slave_cfg.offset_ok = offset_ok;
    slave_cfg.offset_fail = offset_fail;
    return ptp_set_clock_slave_config(&slave_cfg,0) ? 0 : 1;
}
//#ifndef DEBUG
int status_ptp(ptp_clock_default_ds_t *clock_bs,
						vtss_ptp_ext_clock_mode_t *mode,
						vtss_timestamp_t *t,
						ptp_clock_slave_ds_t *clock_slave_bs)
{

    int retval = 0;
    BOOL dataSet = FALSE;
    uint inst;
    u32 hw_time; //not sure what this is for...
    /* Do not want to flood console with warnings */

    /* get the local clock  */

    vtss_ext_clock_out_get(mode);

    inst = 0;
    //For now always use inst 0...
    //	for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++)
    //	{
    //T_D("Inst - %d",inst);
    if ((ptp_get_clock_default_ds(clock_bs,inst))) {
        dataSet = TRUE;
        vtss_local_clock_time_get(t, inst, &hw_time);

        if (!ptp_get_clock_slave_ds(clock_slave_bs,inst)) {
            retval++;
        }
        else
        {
            //	        	break;
        }
    }
    else
    {
        retval++;
    }
    //	}
    if (!dataSet)
    {
        //No entries...
        retval++;
    }

    return retval; // Do not further search the file system.
}
//#endif
int Get_TimeIn_Time(int *year, int *yday, int *hour, int *minute, int *second)
{
	u32 sec_bcd, min_bcd, hour_bcd, day_bcd, year_bcd, tday_bcd;
//	ptp_clock_default_ds_t clock_bs;
//	ptp_clock_slave_ds_t clock_slave_bs;
//	vtss_ptp_ext_clock_mode_t mode;
//	vtss_timestamp_t vt;
//	struct tm tm_t;
	int retval = 0;

	if(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1588)
	{// we are a 1588 slave.
		retval = Get_PTP_SetTime(year, yday, hour, minute, second);
//		if(status_ptp(&clock_bs, &mode, &vt, &clock_slave_bs))
//		{//failed
//			T_W("VNS_FGA: Failed to get ptp status... Try command \".1588 slave\" to resolve this...");
//			retval++;
//		}
//		else
//		{
//			if(0 == Time_t_2_struct_tm_time(vt.seconds, &tm_t))
//			{
//				if(ptp_lock && !debug_test_mode)
//				{
//					*second = tm_t.tm_sec;
//					*minute = tm_t.tm_min;
//					*hour = tm_t.tm_hour;
//					*yday = tm_t.tm_yday;
//					if(tm_t.tm_year < 2000)
//						*year = 0;
//					else
//						*year = tm_t.tm_year - 2000;
//				}
//			}
//		}
	}
	else
	{
		retval += GetRegister(FPGA_IRIG_IN_TIME_SEC, &sec_bcd);
		retval += GetRegister(FPGA_IRIG_IN_TIME_MIN, &min_bcd);
		retval += GetRegister(FPGA_IRIG_IN_TIME_HOUR, &hour_bcd);
		retval += GetRegister(FPGA_IRIG_IN_TIME_DAY, &day_bcd);
		retval += GetRegister(FPGA_IRIG_IN_TIME_TDAY, &tday_bcd);
		retval += GetRegister(FPGA_IRIG_IN_TIME_YEAR, &year_bcd);

		if(retval == 0)
		{
			*second = (sec_bcd & 0xF) + (10 * ((sec_bcd >> 4) & 0xF));
			*minute = (min_bcd & 0xF) + (10 * ((min_bcd >> 4) & 0xF));
			*hour = (hour_bcd & 0xF) + (10 * ((hour_bcd >> 4) & 0xF));
			*yday = (day_bcd & 0xF) + (10 * (tday_bcd & 0xF)) + (100 * ((tday_bcd >> 4) & 0xF));
			*year = 2000 + (year_bcd & 0xF) + (10 * ((year_bcd >> 4) & 0xF));
		}
	}
	return retval;
}

int Get_TimeOut_Time(int *year, int *yday, int *hour, int *minute, int *second)
{
    u32 sec_bcd, min_bcd, hour_bcd, day_bcd;
    int retval = 0;
    retval += GetRegister(FPGA_IRIG_OUT_TIME_SEC, &sec_bcd);
    retval += GetRegister(FPGA_IRIG_OUT_TIME_MIN, &min_bcd);
    retval += GetRegister(FPGA_IRIG_OUT_TIME_HOUR, &hour_bcd);
    retval += GetRegister(FPGA_IRIG_OUT_TIME_DAY, &day_bcd);

    if(retval == 0)
    {
        *second = (sec_bcd & 0xF) + (10 * ((sec_bcd >> 4) & 0xF));
        *minute = (min_bcd & 0xF) + (10 * ((min_bcd >> 4) & 0xF));
        *hour = (hour_bcd & 0xF) + (10 * ((hour_bcd >> 4) & 0xF));
        *yday = (day_bcd & 0xF) + (10 * ((day_bcd >> 4) & 0xF)) + (100 * ((day_bcd >> 8) & 0xF));
        *year = 2000;
    }
    return retval;
}

int modify_input_time(ies_time_process_command_t cmd, u32 seconds)
{
    int retval = 0;
    vns_bcd_time bcd_t;
    struct timeval now;

    gettimeofday(&now, NULL);


    if(cmd == IES_TIME_PROCESS_COMMAND_ADD)
        now.tv_sec += seconds;
    else if(cmd == IES_TIME_PROCESS_COMMAND_SUBTRACT)
        now.tv_sec -= seconds;
    else
        T_W("Can only add and subtract right now");

    cyg_libc_time_settime( now.tv_sec );


    if(0 == Time_t_2_Vns_bcd_time(now.tv_sec , &bcd_t))
    {
        retval += SetRegister(FPGA_1588_TIME_MS, 0);
        retval += SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
        retval += SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
        retval += SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

        retval += SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
        retval += SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
        retval += SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
    }
    return retval;
}
int Get_PTP_SetTime(int *year, int *yday, int *hour, int *minute, int *second)
{
    u32 sec_bcd, min_bcd, hour_bcd, day_bcd, year_bcd, hday_bcd;
    int retval = 0;
    retval += GetRegister(FPGA_1588_TIME_SEC, &sec_bcd);
    retval += GetRegister(FPGA_1588_TIME_MIN, &min_bcd);
    retval += GetRegister(FPGA_1588_TIME_HOUR, &hour_bcd);
    retval += GetRegister(FPGA_1588_TIME_HDAY, &hday_bcd);
    retval += GetRegister(FPGA_1588_TIME_DAY, &day_bcd);
    retval += GetRegister(FPGA_1588_TIME_YEAR, &year_bcd);

    if(retval == 0)
    {
        *second = (sec_bcd & 0xF) + (10 * ((sec_bcd >> 4) & 0xF));
        *minute = (min_bcd & 0xF) + (10 * ((min_bcd >> 4) & 0xF));
        *hour = (hour_bcd & 0xF) + (10 * ((hour_bcd >> 4) & 0xF));
        *yday = (day_bcd & 0xF) + (10 * ((day_bcd >> 4) & 0xF)) + (100 * ((hday_bcd) & 0xF));
        *year = 2000 + (year_bcd & 0xF) + (10 * ((year_bcd >> 4) & 0xF));

        if(PTP_TIMEOFFSET > 0)
        {
            *second -= PTP_TIMEOFFSET;
            if((*second) < 0)
            {
                *second = 59;
                *minute -= 1;
                if((*minute) < 0)
                {
                    *minute = 59;
                    *hour -=1;
                    if((*hour) < 0)
                    {
                        *hour = 23;
                        *yday -= 1;
                        if((*yday <= 0))
                        {
                            *yday = 365;
                            *year -= 1;
                            if((*year) < 0)
                            {
                                *year = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return retval;
}

//int vns_fpga_get_input_time(time_t *tv)
time_t FpgaTime_2_time_t(int year, int yday, int hour, int minute, int second)
{
	int y;
	time_t t;
	long days = -1; //since 0 is day 1
	for(y = 1970; y < year; y++)
	{
		if(y%4 == 0)
		{
			days += 366;
		}
		else
		{
			days += 365;
		}
	}
	days += yday;
	t = 0;
	t += (days * 24 *60 * 60);
	t += (hour *60 * 60);
	t += (minute * 60);
	t += second;
	return t;
}

int Time_t_2_struct_tm_time(time_t t, struct tm * ptime)
{
	int days = 0;
	ptime = localtime(&t); //or gmtime(t)?
	if(ptime == NULL)
		return 1;
	if(ptime->tm_mon >= 2)
	{//add January
		days += 31;
	}
	if(ptime->tm_mon >= 3)
	{//add February
		if((ptime->tm_year % 4) == 0)
			days += 29;
		else
			days += 28;
	}
	if(ptime->tm_mon >= 4)
	{//add March
		days += 31;
	}
	if(ptime->tm_mon >= 5)
	{//add April
		days += 30;
	}
	if(ptime->tm_mon >= 6)
	{//add May
		days += 31;
	}
	if(ptime->tm_mon >= 7)
	{//add June
		days += 30;
	}
	if(ptime->tm_mon >= 8)
	{//add July
		days += 31;
	}
	if(ptime->tm_mon >= 9)
	{//add August
		days += 31;
	}
	if(ptime->tm_mon >= 10)
	{//add September
		days += 30;
	}
	if(ptime->tm_mon >= 11)
	{//add October
		days += 31;
	}
	if(ptime->tm_mon >= 12)
	{//add November
		days += 30;
	}
	days += ptime->tm_mday;
	ptime->tm_yday = days;
	if(ptime->tm_year < 2000)
	{//irig time starts at year 2000
		ptime->tm_year = 2000;
	}
	return 0;

}

int Time_t_2_Vns_bcd_time(time_t t, vns_bcd_time * bcd)
{
	static struct tm gtime;
	struct tm * ptime;
	int days = 0;

	int month_correction = 1;

	ptime = gmtime(&t);//or localtime(&t)?
//	T_D("current time: year: %d, month:%d, day:%d, %02d:%02d:%02d", ptime->tm_year, ptime->tm_mon, ptime->tm_mday,
//			ptime->tm_hour, ptime->tm_min, ptime->tm_sec);

	if(ptime == NULL)
		return 1;

	gtime.tm_year = (ptime->tm_year % 100);
	gtime.tm_yday = ptime->tm_yday;
	gtime.tm_mon = ptime->tm_mon;
	gtime.tm_mday = ptime->tm_mday;
	gtime.tm_hour = ptime->tm_hour;
	gtime.tm_min = ptime->tm_min;
	gtime.tm_sec = ptime->tm_sec;

	gtime.tm_mon += month_correction;

	if(gtime.tm_mon >= 2)
	{//add January
		days += 31;
	}
	if(gtime.tm_mon >= 3)
	{//add February
		if((gtime.tm_year % 4) == 0)
			days += 29;
		else
			days += 28;
	}
	if(gtime.tm_mon >= 4)
	{//add March
		days += 31;
	}
	if(gtime.tm_mon >= 5)
	{//add April
		days += 30;
	}
	if(gtime.tm_mon >= 6)
	{//add May
		days += 31;
	}
	if(gtime.tm_mon >= 7)
	{//add June
		days += 30;
	}
	if(gtime.tm_mon >= 8)
	{//add July
		days += 31;
	}
	if(gtime.tm_mon >= 9)
	{//add August
		days += 31;
	}
	if(gtime.tm_mon >= 10)
	{//add September
		days += 30;
	}
	if(gtime.tm_mon >= 11)
	{//add October
		days += 31;
	}
	if(gtime.tm_mon >= 12)
	{//add November
		days += 30;
	}
	days += gtime.tm_mday;

	bcd->bcd_ts = gtime.tm_sec / 10;
	bcd->bcd_s = gtime.tm_sec % 10;
	bcd->bcd_tm = gtime.tm_min / 10;
	bcd->bcd_m = gtime.tm_min % 10;
	bcd->bcd_th = gtime.tm_hour / 10;
	bcd->bcd_h = gtime.tm_hour % 10;
	bcd->bcd_hd = days / 100;
	bcd->bcd_td = (days % 100) / 10;
	bcd->bcd_d = days % 10;
//	if(gtime.tm_year <= 2000)
//	{
//		bcd->bcd_y = 0;
//		bcd->bcd_ty = 0;
//	}
//	else
//	{
		bcd->bcd_ty = (gtime.tm_year )/ 10;
		bcd->bcd_y = (gtime.tm_year ) % 10;
		if(bcd->bcd_ty > 99)bcd->bcd_ty = 99;
//	}
	return 0;
}
//static void VNS_FPGA_UPGRADE_thread(cyg_addrword_t data)
//{
//		init_fpga_upgrade_ack();
//		UpdateVNSFPGAFirmware(&vns_global.fpga_update.file_info );
//}

vtss_timestamp_t prev_vt;
int timin_health;

/* Pthread variables */
static char fpga_update_pthread_stack[THREAD_DEFAULT_STACK_SIZE * 4];
/* Thread variables */
#define FPGA_UPDATE_CERT_THREAD_STACK_SIZE       8192

#define LED_CERT_THREAD_STACK_SIZE       16384
static cyg_handle_t LED_thread_handle;
static cyg_thread   LED_thread_block;
static char         LED_thread_stack[LED_CERT_THREAD_STACK_SIZE];

int led_thread(void)
{
    if(vns_global.init_complete)
    {
        Set_leds_and_discretes(vns_global.link,
                vns_global.activity,
                vns_global.speed);
    }
    return 0;
}
static void LED_thread(cyg_addrword_t data)
{
    vns_fpga_conf_read(VTSS_ISID_GLOBAL,0);
    if(set_vns_config()) {
        T_E("Failed to set VNS config");
    }
    if(Start_TimeInput()) {
        T_W("VNS FPGA: Failed to start time input!\n");
    }
    if(Start_TimeOutput()) {
        T_W("VNS FPGA: Failed to start time output!\n");
    }
    vns_global.init_complete = 1;
    if(is_mac_default()) {
        T_W("************************************************************");
        T_W("*      USING DEFAULT MAC ADDRESS, PLEASE CHANGE!!          *");
        T_W("************************************************************");
    }
    init_timer(); /* House cleaning timer */
    while (1)
    {
        VTSS_OS_MSLEEP(500);
        T_D("LED thread has started and is looping..");
        if(!led_thread())
        {
            T_D("led_thread out of range.. Starting clean");

        }
    }
}

void StartLED(void)
{
    /* Create DHCP relay thread */
    cyg_thread_create(THREAD_DEFAULT_PRIO,
            LED_thread,
            0,
            "LED",
            LED_thread_stack,
            sizeof(LED_thread_stack),
            &LED_thread_handle,
            &LED_thread_block);
}

void start_fpga_upgrade( update_args_t* args)
{
    pthread_t fpga_update_base_thread;
    pthread_attr_t attr;
    struct sched_param schedparam;
    int rc;

    cancel_timer(); /* Need to stop communication with */
    set_fpga_update_mode( args); /* set flage to make sure pthread is ready */
    /* Create fpga_update thread */
    rc = pthread_attr_init( &attr );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_init fails");
    }

    schedparam.sched_priority = 24;
    rc = pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_setinheritsched fails");
    }
    rc = pthread_attr_setschedpolicy( &attr, SCHED_RR );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_setschedpolicy fails");
    }
    rc = pthread_attr_setschedparam( &attr, &schedparam );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_setschedparam fails");
    }
    rc = pthread_attr_setstackaddr( &attr, (void *)&fpga_update_pthread_stack[sizeof(fpga_update_pthread_stack)] );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_setstackaddr fails");
    }
    rc = pthread_attr_setstacksize( &attr, sizeof(fpga_update_pthread_stack) );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_attr_setstacksize fails");
    }

    rc = pthread_create( &fpga_update_base_thread, &attr, UpdateVNSFPGAFirmware, (void*)&vns_global.fpga_update.file_info );
    if (rc != 0) {
        T_W("fpga_update_create_pthread: pthread_create fails");
    } else {
        T_D( "UpdateVNSFPGAFirmware\n");
    }

    while(!is_fpga_update_ready()){
        cyg_thread_delay(CYG_MSEC2TICK(1));
    }

    return;
}
void init_fpga_upgrade_ack(void)
{
	vns_global.fpga_update.must_update = 0;
}

void set_quick_reference_variables(void)
{
    if (conf_mgmt_board_get(&vns_global.board_conf) != 0) 
        T_E("Board set operation failed\n");
    if(0 != Get_FPGAFirmwareVersionMinor(&vns_global.fw_minor) || 0 != Get_FPGAFirmwareVersionMajor(&vns_global.fw_major) ) 
        T_E("Could not get firmware version\n");
}
static void vns_fpga_init_cmd_start(vtss_timer_t *timer)
{

    vns_fpga_conf_read(VTSS_ISID_GLOBAL,0);
    if(set_vns_config()) {
        T_E("Failed to set VNS config");
    }
    set_quick_reference_variables();
    if(Start_TimeInput()) {
        T_W("VNS FPGA: Failed to start time input!\n");
    }
    if(Start_TimeOutput()) {
        T_W("VNS FPGA: Failed to start time output!\n");
    }
    vns_global.init_complete = 1;
    init_timer(); /* House cleaning timer */
    init_led_discrete_timer();
    if(is_mac_default()) {
        T_W("************************************************************");
        T_W("*      USING DEFAULT MAC ADDRESS, PLEASE CHANGE!!          *");
        T_W("************************************************************");
    }
}
static void led_discrete_timer(vtss_timer_t *timer)
{
    /*
     * Defect: 3431
     * LEDs continued to flash after the boot sequence was complete. Set_leds_and_discretes(0,0,0) at this point fixes it
     *
     */
    if(vns_global.init_complete)
    {
        Set_leds_and_discretes(vns_global.link,
                vns_global.activity,
                vns_global.speed);
        if(adjust_leap_seconds())
        {
            T_W("Was not able to adjust Leap seconds!");
        }
    }

}
static void debug_timer(vtss_timer_t *timer)
{
        /* debug_cnt ++; */
        /* if(debug_cnt %100) { */
        /*     T_W("IES_DEBUGGING is enabled"); */
        /* } */
        T_W("IES_DEBUGGING is enabled");
        T_W("Please update to non-debug load");
}
static void no_ptp_clk_warning()
{
    fail_count++;
    if(fail_count % 10 == 0)
        T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
}

static void vns_fpga_timer_ptp_disable(vtss_timer_t *timer)
{
    struct timeval now;
    /* For PTP work */
    struct tm gtime;
    time_t t;
    //	BOOL set_time = FALSE;
    //#ifdef DEBUG
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    uint32_t time_in_health;
    /* Shared for both */
    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "PTP DISABLED ");
    u32 rtc;
    if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
        //failed
        ptp_lock = FALSE;
    } else {
        if(timer->period_us != 1000000)
            timer->period_us = 1000000; //change to one second interval
        // there is a PTP clock running... set system time if it's locked...
        gettimeofday(&now, NULL);
        //    	gmtime(&(now.tv_sec));

        ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);
        // if the PPS mode is set, un-set it
        if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_DISABLE) {
            mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
            vtss_ext_clock_out_set(&mode);
        }

        GetTimeInHealth(&time_in_health);

        if(ptp_lock){
            if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)){
                T_W("1 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                cyg_libc_time_settime(ptp_time.seconds);
            } else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
                T_W("2 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                cyg_libc_time_settime(ptp_time.seconds);
            }
        }else if(0 == time_in_health){
            if(0 == Get_TimeIn_Time(&gtime.tm_year, &gtime.tm_yday, &gtime.tm_hour, &gtime.tm_min, &gtime.tm_sec)) {
                t = FpgaTime_2_time_t(gtime.tm_year, gtime.tm_yday, gtime.tm_hour, gtime.tm_min, gtime.tm_sec);

                if((now.tv_sec > t) && ((now.tv_sec - t) > 1)){
                    T_W("3 Setting System Time to %u, was %u", t, now.tv_sec);
                    cyg_libc_time_settime(t);
                } else if((now.tv_sec < t) && ((t - now.tv_sec) > 1)) {
                    T_W("4 Setting System Time to %u, was %u", t, now.tv_sec);
                    cyg_libc_time_settime(t);
                }
            }
        }

    }
}

static void vns_fpga_timer_slave(vtss_timer_t *timer)
{
    struct timeval now;
    /* For PTP work */
    static BOOL slave_cfg_set = false;
    struct tm gtime;
    time_t t;
    //	BOOL set_time = FALSE;
    //#ifdef DEBUG
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    uint32_t time_in_health;
    /* Shared for both */
    vns_bcd_time bcd_t;
    u32 time_out_enabled;
    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "PTP SLAVE MODE");

    	if(!slave_cfg_set) {
    		// todo: make these config parameters, allow changing in 1588 webgui as well as cli
    		if(0 == set_ptp_slave_cfg(500, 1000, 2000)){
    			slave_cfg_set = true;
    		}
    	}
        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
            //failed
            //T_W("VNS_FGA: Failed to get ptp status... Attempting to resolve this...");
            if(timer->period_us != 1000000)
            {
                timer->period_us = 1000000;
                T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
            }
            no_ptp_clk_warning();
            ptp_lock = FALSE;
        }else {
            if(timer->period_us != 100000) {
                timer->period_us = 100000; //change to 1/10 second interval
            }
            ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);
            if(ptp_lock){
            	if(config_shaddow.time_out_setting.timecode != TIME_OUTPUT_TYPE_DISABLED) {
            		GetRegister(FPGA_IRIG_OUT_ENABLE, &time_out_enabled);
            		if(0 == time_out_enabled){
            			Start_TimeOutput();
            		}
            	}
            } else {
            	Stop_TimeOutput();
            }

#if defined(BOARD_IGU_REF)
            /* we are faking the registers, so we need to update the HEALTH register */
            if(ptp_lock) {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0);
            } else {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0x18);
            }
#endif

            if((prev_vt.seconds != ptp_time.seconds) && (ptp_time.nanoseconds > 200000000)) {
                if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_OUTPUT) {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_OUTPUT;
                    vtss_ext_clock_out_set(&mode);
                    //T_W("1PPS OUTPUT MUST BE ENABLED IN FOR PTP IN ORDER TO PROPERLY ALLIGN IRIG TIME CODES TO IEEE-1588!");
                }
                if(0 == Time_t_2_Vns_bcd_time(ptp_time.seconds + PTP_TIMEOFFSET, &bcd_t)) {
                    if(/*ptp_lock &&*/ !debug_test_mode) {
                        SetRegister(FPGA_1588_TIME_MS, 0);
                        SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
                        SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
                        SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

                        SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
                        SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
                        SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
                    }
                }
            	gettimeofday(&now, NULL);
        //    	gmtime(&(now.tv_sec));
                if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)) {
                	// set system time
                	T_W("1 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                	cyg_libc_time_settime(ptp_time.seconds);
                } else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
                	T_W("2 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
        			cyg_libc_time_settime(ptp_time.seconds);
        		}
                prev_vt.seconds = ptp_time.seconds;
            }
        }

}
static void vns_fpga_timer_master(vtss_timer_t *timer)
{
    static int prevSec = 0;
    /* For PTP work */
    static BOOL slave_cfg_set = false;
    struct tm gtime;
    time_t t;
    //	BOOL set_time = FALSE;
    //#ifdef DEBUG
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    uint32_t time_in_health;
    /* Shared for both */
    u32 rtc;

    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "1588 Master");
    ptp_lock = FALSE;
    slave_cfg_set = FALSE;
    GetRegister(FPGA_IRIG_IN_RTC_VAL_LSW, &rtc);//clears the interrupt
    // set the time in the ptp core if it is not current
    if(0 == Get_TimeIn_Time(&gtime.tm_year, &gtime.tm_yday, &gtime.tm_hour, &gtime.tm_min, &gtime.tm_sec)) {
        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
            //failed... change to 1 second interval
            if(timer->period_us != 1000000)
            {
                timer->period_us = 1000000;
                T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
            }
            no_ptp_clk_warning();
        } else {
            // get ptp status succeeded... change to 1/10 second interval
            if(timer->period_us != 100000) {
                timer->period_us = 100000;
            }
            GetTimeInHealth(&time_in_health);

            if(0 == time_in_health) {
                // we are locked and should have a decent 1pps
                if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_INPUT) {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_INPUT;
                    vtss_ext_clock_out_set(&mode);
                }
                t = FpgaTime_2_time_t(gtime.tm_year, gtime.tm_yday, gtime.tm_hour, gtime.tm_min, gtime.tm_sec);

                if(ptp_time.seconds != t)
                {//The PTP time is not set to input time.. set the ptp time
                    if(ptp_time.nanoseconds > 10000000 && ptp_time.nanoseconds < 300000000) {
                        if((ptp_time.seconds - t) < 10)	{
                            //reset the switch to 0 (for some reason it does not like to be set to a close number)
                            //this forces a full time reset.
                            ptp_time.seconds = 0;
                            ptp_time.nanoseconds = 0;
                            vtss_local_clock_time_set(&ptp_time, 0);
                        }
                        ptp_time.seconds = t;
                        vtss_local_clock_time_set(&ptp_time, 0);
                        cyg_libc_time_settime( ptp_time.seconds );
                        T_W("Switch time changed to %s ", misc_time2str(ptp_time.seconds));
                    }
                }
            } else {
                //time input is freewheeling, don't use the pps input, and let PTP clock freewheel
                if(mode.one_pps_mode == VTSS_PTP_ONE_PPS_INPUT) {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                    vtss_ext_clock_out_set(&mode);
                }
            }
        }
    }
}
static void vns_fpga_timer_handle_1pps(vtss_timer_t *timer)
{ /* For 1PPS mode */
    int retval = 0;
    struct timeval now;
    static int prevSec = 0;
    /* Shared for both */
    vns_bcd_time bcd_t;

    if ( config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1PPS)
    {

        /*
         * Need this for IRIG out
         */
        /* if(config_shaddow.time_out_setting.ieee_1588_type != IEEE_1588_SLAVE){ */
            /* Set_IEEE_1588_Mode(IEEE_1588_SLAVE); */
        /* } */

        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "1PPS set. ");
        if(timer->period_us != 100000) {
            timer->period_us = 100000;
        }
        /* get system time */
        /* set 1588 registers  */
        /* vtss_local_clock_time_get(&pps_time, inst, &hw_time); */
        gettimeofday(&now, NULL);

        /* see whether any source routing information has expired */
        if(now.tv_sec != prevSec){
            /* T_W("Setting 1PPS time to %u", now.tv_sec); */
            T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER,"Setting 1PPS time to %u", now.tv_sec);
            if(0 == Time_t_2_Vns_bcd_time(now.tv_sec , &bcd_t))
            {
                retval += SetRegister(FPGA_1588_TIME_MS, 0);
                retval += SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
                retval += SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
                retval += SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

                retval += SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
                retval += SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
                retval += SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
            }
            prevSec = now.tv_sec;
        }
    }

}
static void vns_fpga_timer_cb(vtss_timer_t *timer)
{
    /* For 1PPS mode */
    int retval = 0;
    struct timeval now;
    static int prevSec = 0;
    /* For PTP work */
    static BOOL slave_cfg_set = false;
    struct tm gtime;
    time_t t;
    //	BOOL set_time = FALSE;
    //#ifdef DEBUG
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    uint32_t time_in_health;
    /* Shared for both */
    vns_bcd_time bcd_t;
    u32 time_out_enabled;
    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Enter Timer");
#if defined( BOARD_VNS_8_REF )
    ies_8_update_time_lock_status(timer);
    return;
#endif
    if ( (config_shaddow.time_in_setting.source == TIME_INPUT_SRC_DISABLED) )
    {
        if(timer->period_us != 1000000) {
            timer->period_us = 1000000;
        }
        return;
    }

    /*
     * Need to disable timer for i2c bus similarly to disabling interrupts.
     */
    if(vns_global.fpga_update.disable_timer ) {
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Timer Disabled");
        return;
    }

    int fpga_irq = 0;
    u32 rtc;

    if ( config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1PPS)
    {
        vns_fpga_timer_handle_1pps(timer);
        return;
    }


    // MAKE SURE 1588 SLAVE MODE IS USED IF TIME INPUT SOURCE IS 1588
    if(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1588){ 
        if(config_shaddow.time_out_setting.ieee_1588_type != IEEE_1588_SLAVE){
    		config_shaddow.time_out_setting.ieee_1588_type = IEEE_1588_SLAVE;
    	}
    }

    // CHECK TIME IRQ STATUS
    if(GetRegister(FPGA_IRIG_IN_INTERRUPT, &fpga_irq)) {
    	//failed to read register...
        fpga_irq = 0;
    } else {
    	if(fpga_irq) {
    		//clear the interrupt
    		GetRegister(FPGA_IRIG_IN_RTC_VAL_LSW, &rtc);
    	}
    }

    switch( config_shaddow.time_out_setting.ieee_1588_type )
    {
    case IEEE_1588_MASTER:
#if !(defined(BOARD_NCGU_REF) || defined(BOARD_IGU_REF)) /* igu and ncgu cannot be master clocks */
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "1588 Master");
        ptp_lock = FALSE;
        slave_cfg_set = FALSE;
        if(fpga_irq) {

            vns_fpga_timer_master(timer);
        }
#endif
    	break;// end of 1588 master mode
    case IEEE_1588_SLAVE:
        vns_fpga_timer_slave(timer);
    	break;// end of 1588 slave mode
    default:
        vns_fpga_timer_ptp_disable(timer);
    	break; // end of 1588 mode disabled
        //create_ptp_clock(true);
    }
}

static void vns_fpga_timer_cb_org(vtss_timer_t *timer)
{
    /* For 1PPS mode */
    int retval = 0;
    struct timeval now;
    static int prevSec = 0;
    /* For PTP work */
    static BOOL slave_cfg_set = false;
    struct tm gtime;
    time_t t;
    //	BOOL set_time = FALSE;
    //#ifdef DEBUG
    ptp_clock_default_ds_t clock_bs;
    ptp_clock_slave_ds_t clock_slave_bs;
    vtss_ptp_ext_clock_mode_t mode;
    uint32_t time_in_health;
    /* Shared for both */
    vns_bcd_time bcd_t;
    u32 time_out_enabled;
    T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Enter Timer");
    if ( (config_shaddow.time_in_setting.source == TIME_INPUT_SRC_DISABLED) )
    {
        if(timer->period_us != 1000000) {
            timer->period_us = 1000000;
        }
        return;
    }

    /*
     * Need to disable timer for i2c bus similarly to disabling interrupts.
     */
    if(vns_global.fpga_update.disable_timer ) {
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Timer Disabled");
        return;
    }

    int fpga_irq = 0;
    u32 rtc;

    if ( config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1PPS)
    {
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "1PPS set. ");
        if(timer->period_us != 100000) {
            timer->period_us = 100000;
        }
    	/* get system time */
    	/* set 1588 registers  */
    	/* vtss_local_clock_time_get(&pps_time, inst, &hw_time); */
    	gettimeofday(&now, NULL);

    	/* see whether any source routing information has expired */
    	if(now.tv_sec != prevSec){
    		/* T_W("Setting 1PPS time to %u", now.tv_sec); */
                T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER,"Setting 1PPS time to %u", now.tv_sec);
    		if(0 == Time_t_2_Vns_bcd_time(now.tv_sec , &bcd_t))
    		{
    			retval += SetRegister(FPGA_1588_TIME_MS, 0);
    			retval += SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
    			retval += SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
    			retval += SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

    			retval += SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
    			retval += SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
    			retval += SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
    		}
    		prevSec = now.tv_sec;
    	}
    }


    // MAKE SURE 1588 SLAVE MODE IS USED IF TIME INPUT SOURCE IS 1588
    if(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1588){ 
        if(config_shaddow.time_out_setting.ieee_1588_type != IEEE_1588_SLAVE){
    		config_shaddow.time_out_setting.ieee_1588_type = IEEE_1588_SLAVE;
    	}
    }

    // CHECK TIME IRQ STATUS
    if(GetRegister(FPGA_IRIG_IN_INTERRUPT, &fpga_irq)) {
    	//failed to read register...
        fpga_irq = 0;
    } else {
    	if(fpga_irq) {
    		//clear the interrupt
    		GetRegister(FPGA_IRIG_IN_RTC_VAL_LSW, &rtc);
    	}
    }

    switch( config_shaddow.time_out_setting.ieee_1588_type )
    {
    case IEEE_1588_MASTER:
#if !(defined(BOARD_NCGU_REF) || defined(BOARD_IGU_REF)) /* igu and ncgu cannot be master clocks */
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "1588 Master");
        ptp_lock = FALSE;
        slave_cfg_set = FALSE;
        if(fpga_irq) {
            GetRegister(FPGA_IRIG_IN_RTC_VAL_LSW, &rtc);//clears the interrupt
            // set the time in the ptp core if it is not current
            if(0 == Get_TimeIn_Time(&gtime.tm_year, &gtime.tm_yday, &gtime.tm_hour, &gtime.tm_min, &gtime.tm_sec)) {
                if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
                	//failed... change to 1 second interval
                    if(timer->period_us != 1000000)
                    {
                        timer->period_us = 1000000;
                        T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
                    }
                    no_ptp_clk_warning();
                } else {
                	// get ptp status succeeded... change to 1/10 second interval
                    if(timer->period_us != 100000) {
                        timer->period_us = 100000;
                    }
                    GetTimeInHealth(&time_in_health);

                    if(0 == time_in_health) {
                    	// we are locked and should have a decent 1pps
                    	if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_INPUT) {
                    		mode.one_pps_mode = VTSS_PTP_ONE_PPS_INPUT;
                    		vtss_ext_clock_out_set(&mode);
                    	}
                        t = FpgaTime_2_time_t(gtime.tm_year, gtime.tm_yday, gtime.tm_hour, gtime.tm_min, gtime.tm_sec);

    					if(ptp_time.seconds != t)
    					{//The PTP time is not set to input time.. set the ptp time
    						if(ptp_time.nanoseconds > 10000000 && ptp_time.nanoseconds < 300000000) {
    							if((ptp_time.seconds - t) < 10)	{
    								//reset the switch to 0 (for some reason it does not like to be set to a close number)
    								//this forces a full time reset.
    								ptp_time.seconds = 0;
    								ptp_time.nanoseconds = 0;
    								vtss_local_clock_time_set(&ptp_time, 0);
    							}
    							ptp_time.seconds = t;
    							vtss_local_clock_time_set(&ptp_time, 0);
    							cyg_libc_time_settime( ptp_time.seconds );
    							T_W("Switch time changed to %s ", misc_time2str(ptp_time.seconds));
    						}
    					}
                    } else {
                    	//time input is freewheeling, don't use the pps input, and let PTP clock freewheel
                        if(mode.one_pps_mode == VTSS_PTP_ONE_PPS_INPUT) {
                            mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                            vtss_ext_clock_out_set(&mode);
                        }
                    }
                }
            }
        }
#endif
    	break;// end of 1588 master mode
    case IEEE_1588_SLAVE:
    	if(!slave_cfg_set) {
    		// todo: make these config parameters, allow changing in 1588 webgui as well as cli
    		if(0 == set_ptp_slave_cfg(500, 1000, 2000)){
    			slave_cfg_set = true;
    		}
    	}
        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
            //failed
            //T_W("VNS_FGA: Failed to get ptp status... Attempting to resolve this...");
            if(timer->period_us != 1000000)
            {
                timer->period_us = 1000000;
                T_W("PTP not active when 1588 config is active. Must create PTP clock inst 0.");
            }
            no_ptp_clk_warning();
            ptp_lock = FALSE;
        }else {
            if(timer->period_us != 100000) {
                timer->period_us = 100000; //change to 1/10 second interval
            }
            ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);
            if(ptp_lock){
            	if(config_shaddow.time_out_setting.timecode != TIME_OUTPUT_TYPE_DISABLED) {
            		GetRegister(FPGA_IRIG_OUT_ENABLE, &time_out_enabled);
            		if(0 == time_out_enabled){
            			Start_TimeOutput();
            		}
            	}
            } else {
            	Stop_TimeOutput();
            }

#if defined(BOARD_IGU_REF)
            /* we are faking the registers, so we need to update the HEALTH register */
            if(ptp_lock) {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0);
            } else {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0x18);
            }
#endif

            if((prev_vt.seconds != ptp_time.seconds) && (ptp_time.nanoseconds > 200000000)) {
                if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_OUTPUT) {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_OUTPUT;
                    vtss_ext_clock_out_set(&mode);
                    //T_W("1PPS OUTPUT MUST BE ENABLED IN FOR PTP IN ORDER TO PROPERLY ALLIGN IRIG TIME CODES TO IEEE-1588!");
                }
                if(0 == Time_t_2_Vns_bcd_time(ptp_time.seconds + PTP_TIMEOFFSET, &bcd_t)) {
                    if(/*ptp_lock &&*/ !debug_test_mode) {
                        SetRegister(FPGA_1588_TIME_MS, 0);
                        SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
                        SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
                        SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

                        SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
                        SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
                        SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
                    }
                }
            	gettimeofday(&now, NULL);
        //    	gmtime(&(now.tv_sec));
                if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)) {
                	// set system time
                	T_W("1 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                	cyg_libc_time_settime(ptp_time.seconds);
                } else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
                	T_W("2 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
        			cyg_libc_time_settime(ptp_time.seconds);
        		}
                prev_vt.seconds = ptp_time.seconds;
            }
        }
    	break;// end of 1588 slave mode
    default:
        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs)) {
        	//failed
        	ptp_lock = FALSE;
        } else {
            if(timer->period_us != 1000000)
                timer->period_us = 1000000; //change to one second interval
        	// there is a PTP clock running... set system time if it's locked...
        	gettimeofday(&now, NULL);
    //    	gmtime(&(now.tv_sec));

        	ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);
        	// if the PPS mode is set, un-set it
            if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_DISABLE) {
                mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                vtss_ext_clock_out_set(&mode);
            }

            GetTimeInHealth(&time_in_health);

        	if(ptp_lock){
        		if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)){
        			T_W("1 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
        			cyg_libc_time_settime(ptp_time.seconds);
        		} else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
        			T_W("2 Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
        			cyg_libc_time_settime(ptp_time.seconds);
        		}
        	}else if(0 == time_in_health){
        		if(0 == Get_TimeIn_Time(&gtime.tm_year, &gtime.tm_yday, &gtime.tm_hour, &gtime.tm_min, &gtime.tm_sec)) {
                    t = FpgaTime_2_time_t(gtime.tm_year, gtime.tm_yday, gtime.tm_hour, gtime.tm_min, gtime.tm_sec);

        			if((now.tv_sec > t) && ((now.tv_sec - t) > 1)){
        				T_W("3 Setting System Time to %u, was %u", t, now.tv_sec);
        				cyg_libc_time_settime(t);
        			} else if((now.tv_sec < t) && ((t - now.tv_sec) > 1)) {
        				T_W("4 Setting System Time to %u, was %u", t, now.tv_sec);
        				cyg_libc_time_settime(t);
        			}
        		}
        	}

        }
    	break; // end of 1588 mode disabled
        //create_ptp_clock(true);
    }
#if 0
    //if we are in 1588 slave mode, set current time to the fpga (as soon as possible to the rollover of the second)
    if(config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1588)
    {// we are a 1588 slave.

    	gettimeofday(&now, NULL);
//    	gmtime(&(now.tv_sec));

        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs))
        {//failed
            T_W("VNS_FGA: Failed to get ptp status... Attempting to resolve this...");
            if(timer->period_us != 1000000)
                timer->period_us = 1000000; //change to one second interval for this error mode
            ptp_lock = FALSE;
        }
        else
        {
            if(timer->period_us != 100000)
                timer->period_us = 100000; //change to 1/10 second interval

            ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);

#if defined(BOARD_IGU_REF)
            /* we are faking the registers, so we need to update the HEALTH register */
            if(ptp_lock)
            {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0);
            }
            else
            {
                SetRegister(FPGA_IRIG_IN_HEALTH, 0x18);
            }
#endif

            if(prev_vt.seconds != ptp_time.seconds)
            {
                if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_OUTPUT)
                {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_OUTPUT;
                    vtss_ext_clock_out_set(&mode);
                    //T_W("1PPS OUTPUT MUST BE ENABLED IN FOR PTP IN ORDER TO PROPERLY ALLIGN IRIG TIME CODES TO IEEE-1588!"); 
                }
                if(0 == Time_t_2_Vns_bcd_time(ptp_time.seconds + PTP_TIMEOFFSET, &bcd_t))
                {
                    if(/*ptp_lock &&*/ !debug_test_mode)
                    {
                        SetRegister(FPGA_1588_TIME_MS, 0);
                        SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
                        SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
                        SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

                        SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
                        SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
                        SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
                    }
                }
                if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)) {
                	// set system time
                	T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                	cyg_libc_time_settime(ptp_time.seconds);
                } else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
                	T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
        			cyg_libc_time_settime(ptp_time.seconds);
        		}
                prev_vt.seconds = ptp_time.seconds;
            }
        }
    }
    else if ( config_shaddow.time_in_setting.source == TIME_INPUT_SRC_1PPS)
    {
        /* get system time */
        /* set 1588 registers  */
        /* vtss_local_clock_time_get(&pps_time, inst, &hw_time); */
        gettimeofday(&now, NULL);

        /* see whether any source routing information has expired */

        /* if(0 == Time_t_2_Vns_bcd_time(pps_time.seconds , &bcd_t)) */
        if(0 == Time_t_2_Vns_bcd_time(now.tv_sec , &bcd_t))
        {
            retval += SetRegister(FPGA_1588_TIME_MS, 0);
            retval += SetRegister(FPGA_1588_TIME_SEC, ((bcd_t.bcd_ts << 4) | bcd_t.bcd_s));
            retval += SetRegister(FPGA_1588_TIME_MIN, ((bcd_t.bcd_tm << 4) | bcd_t.bcd_m));
            retval += SetRegister(FPGA_1588_TIME_HOUR, ((bcd_t.bcd_th << 4) | bcd_t.bcd_h));

            retval += SetRegister(FPGA_1588_TIME_HDAY, bcd_t.bcd_hd);
            retval += SetRegister(FPGA_1588_TIME_DAY, ((bcd_t.bcd_td << 4) | (bcd_t.bcd_d)));
            retval += SetRegister(FPGA_1588_TIME_YEAR, ((bcd_t.bcd_ty << 4) | (bcd_t.bcd_y)));
        }
        /* ************************************************************ */
        //we may be a 1588 master, either way, we need to set our time.
        ptp_lock = FALSE;

        // set the time in the ptp core if it is not current
        if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs))
        {//failed
            //					T_W("VNS_FGA: Failed to get ptp status");
            if(config_shaddow.time_out_setting.ieee_1588_type == IEEE_1588_MASTER)
            {
                //create_ptp_clock(true);
            }
            if(timer->period_us != 1000000)
                timer->period_us = 1000000; //change to 1 second interval
        }
        else
        {
            if(timer->period_us != 100000)
                timer->period_us = 100000; //change to 1/10 second interval

            GetTimeInHealth(&time_in_health);

            if(0 == time_in_health)
            {// we are locked and should have a decent 1pps
                if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_INPUT &&
                		(config_shaddow.time_out_setting.ieee_1588_type == IEEE_1588_MASTER))
                {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_INPUT;
                    vtss_ext_clock_out_set(&mode);
                }
            }
            else
            {//time input is freewheeling
                if(mode.one_pps_mode == VTSS_PTP_ONE_PPS_INPUT)
                {
                    mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                    vtss_ext_clock_out_set(&mode);
                }
            }
            if(config_shaddow.time_out_setting.ieee_1588_type == IEEE_1588_MASTER){
				if(ptp_time.seconds != now.tv_sec)
				{//resetting the time
					if(ptp_time.nanoseconds > 10000000 && ptp_time.nanoseconds < 300000000)
					{
						ptp_time.seconds = now.tv_sec;//+1; //looks like the clock starts on the next 1pps
						vtss_local_clock_time_set(&ptp_time, 0);
						T_W("Switch time changed to %s ", misc_time2str(ptp_time.seconds));
					}
				}
            }
        }


        /* ************************************************************ */
    }
#if !(defined(BOARD_NCGU_REF) || defined(BOARD_IGU_REF))
    else
    {//we may be a 1588 master, either way, we need to set our time.
        ptp_lock = FALSE;
        if(GetRegister(FPGA_IRIG_IN_INTERRUPT, &fpga_irq))
        {//failed to read register...
            fpga_irq = 0;
        }
        if(fpga_irq)
        {
            GetRegister(FPGA_IRIG_IN_RTC_VAL_LSW, &rtc);//clears the interrupt

            gettimeofday(&now, NULL);
//            gmtime(&(now.tv_sec));

            // set the time in the ptp core if it is not current
            if(0 == Get_TimeIn_Time(&gtime.tm_year, &gtime.tm_yday, &gtime.tm_hour, &gtime.tm_min, &gtime.tm_sec))
            {
                if(status_ptp(&clock_bs, &mode, &ptp_time, &clock_slave_bs))
                {//failed
                    if(timer->period_us != 1000000)
                        timer->period_us = 1000000; //change to 1 second interval
                }
                else
                {
                    if(timer->period_us != 100000)
                        timer->period_us = 100000; //change to 1/10 second interval

                    ptp_lock = (clock_slave_bs.slave_state == VTSS_PTP_SLAVE_CLOCK_STATE_LOCKED);

                    GetTimeInHealth(&time_in_health);

                    if(0 == time_in_health)
                    {// we are locked and should have a decent 1pps
                    	if(config_shaddow.time_out_setting.ieee_1588_type == IEEE_1588_MASTER) {
							if(mode.one_pps_mode != VTSS_PTP_ONE_PPS_INPUT) {
								mode.one_pps_mode = VTSS_PTP_ONE_PPS_INPUT;
								vtss_ext_clock_out_set(&mode);
							}
                    	} else {
                    		mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                    		vtss_ext_clock_out_set(&mode);
                    	}
                    }
                    else
                    {//time input is freewheeling
                        if(mode.one_pps_mode == VTSS_PTP_ONE_PPS_INPUT)
                        {
                            mode.one_pps_mode = VTSS_PTP_ONE_PPS_DISABLE;
                            vtss_ext_clock_out_set(&mode);
                        }
                    }

                    t = FpgaTime_2_time_t(gtime.tm_year, gtime.tm_yday, gtime.tm_hour, gtime.tm_min, gtime.tm_sec);

                    if(config_shaddow.time_out_setting.ieee_1588_type == IEEE_1588_MASTER) {
                    	if(ptp_time.seconds != t)
                    	{//resetting the time
                    		if(ptp_time.nanoseconds > 10000000 && ptp_time.nanoseconds < 300000000)
                    		{
                    			if((ptp_time.seconds - t) < 10)
                    			{//reset the switch to 0 (for some reason it does not like to be set to a close number)
                    				//this forces a full time reset.
                    				ptp_time.seconds = 0;
                    				ptp_time.nanoseconds = 0;
                    				vtss_local_clock_time_set(&ptp_time, 0);
                    			}
								ptp_time.seconds = t;//+1; //looks like the clock starts on the next 1pps
								//ptp_time.nanoseconds = 0;
								vtss_local_clock_time_set(&ptp_time, 0);
								cyg_libc_time_settime( ptp_time.seconds );
								T_W("Switch time changed to %s ", misc_time2str(ptp_time.seconds));

                    		}
                    	}
                    } else {
                    	// we are not a master clock, but if any time source is locked, set the system time.
                    	if(ptp_lock){
                    		if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)){
                    			T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                    			cyg_libc_time_settime(ptp_time.seconds);
                    		} else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
                    			T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
                    			cyg_libc_time_settime(ptp_time.seconds);
                    		}
                    	}else if(0 == time_in_health){
                    		//cyg_libc_time_settime( t );
							ptp_time.seconds = t;
							ptp_time.nanoseconds = 0;
							if((now.tv_sec > ptp_time.seconds) && ((now.tv_sec - ptp_time.seconds) > 1)){
								T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
								cyg_libc_time_settime(ptp_time.seconds);
							} else if((now.tv_sec < ptp_time.seconds) && ((ptp_time.seconds - now.tv_sec) > 1)) {
								T_W("Setting System Time to %u, was %u", ptp_time.seconds, now.tv_sec);
								cyg_libc_time_settime(ptp_time.seconds );
							}
                    	}
                    }
                }
            }

        }

    }
#endif //!(defined(BOARD_NCGU_REF) || defined(BOARD_IGU_REF))

#endif /* 0 */

}

static void cancel_timer(void)
{
        T_DG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Cancel Timer");
        if (vtss_timer_cancel(&vns_global.ptp_timer) != VTSS_RC_OK) {
            T_WG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to cancel vns ptp_timer");
        }
	   /* if (vtss_timer_cancel(&vns_global.led_timer) != VTSS_RC_OK) { */
	   /*      T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to cancel vns led_timer"); */
	   /*  } */
}

static void init_timer(void) 
{
    // Create ptp timer
//    static vtss_timer_t timer;
    if (vtss_timer_initialize(&vns_global.ptp_timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to initialize vns ptp_timer");
    }
    vns_global.ptp_timer.repeat      = TRUE;
#ifdef IES_DEBUGGING
    /* making it slower for debug messages */
    vns_global.ptp_timer.period_us   = 3000000;     /* 50000 us = 1/20 sec */
#else
    vns_global.ptp_timer.period_us   = 100000;     /* 50000 us = 1/20 sec */
#endif
    vns_global.ptp_timer.dsr_context = FALSE;
    vns_global.ptp_timer.callback    = vns_fpga_timer_cb;
    vns_global.ptp_timer.prio        = VTSS_TIMER_PRIO_NORMAL;
    vns_global.ptp_timer.modid       = VTSS_MODULE_ID_VNS_FPGA;
    if (vtss_timer_start(&vns_global.ptp_timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to start vns ptp_timer");
    }
}
static void init_setup_timer(void) 
{
    // Schedule delayed setup timer
    static vtss_timer_t timer;
    if (vtss_timer_initialize(&timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to initialize vns setup timer");
    }
    timer.repeat      = FALSE;
    timer.period_us   = 90000;     /* 50000 us = 1/20 sec */
    timer.dsr_context = FALSE;
    timer.callback    = vns_fpga_init_cmd_start;
    timer.prio        = VTSS_TIMER_PRIO_HIGH;
    timer.modid       = VTSS_MODULE_ID_VNS_FPGA;
    if (vtss_timer_start(&timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to start vns setup timer");
    }
}
static void init_led_discrete_timer(void) 
{
    // Schedule delayed setup timer
    /* static vtss_timer_t timer; */
    if (vtss_timer_initialize(&vns_global.led_timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to initialize vns setup timer");
    }
    vns_global.led_timer.repeat      = TRUE;
    vns_global.led_timer.period_us   = 500000;     /* 50000 us = 1/20 sec */
    vns_global.led_timer.dsr_context = FALSE;
    vns_global.led_timer.callback    = led_discrete_timer;
    vns_global.led_timer.prio        = VTSS_TIMER_PRIO_HIGH;
    vns_global.led_timer.modid       = VTSS_MODULE_ID_VNS_FPGA;
    if (vtss_timer_start(&vns_global.led_timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to start vns setup timer");
    }
}
static void init_debug_timer(void) 
{
    // Schedule delayed setup timer
    static vtss_timer_t timer;
    if (vtss_timer_initialize(&timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to initialize vns setup timer");
    }
    timer.repeat      = TRUE;
    timer.period_us   = 90000000;     /* 50000 us = 1/20 sec */
    timer.dsr_context = FALSE;
    timer.callback    = debug_timer;
    timer.prio        = VTSS_TIMER_PRIO_LOW;
    timer.modid       = VTSS_MODULE_ID_VNS_FPGA;
    if (vtss_timer_start(&timer) != VTSS_RC_OK) {
        T_EG(VTSS_TRACE_GRP_VNS_BASE_TIMER, "Unable to start vns setup timer");
    }
}
void init_led_values(void)
{
    vns_global.link     = 0;
    vns_global.activity = 0;
    vns_global.speed    = 0;
}
/****************************************************************************/
// vns_fpga_init()
// Module initialization function.
/****************************************************************************/
vtss_rc vns_fpga_init(vtss_init_data_t *data)
{

	vtss_isid_t isid = data->isid;
	vns_global.fpga_update.disable_timer = 0;

//#if defined (BOARD_VNS_6_REF)
	/************************************************************/
	/******************* trying to delay for debug ********************/
//	unsigned long counter;
//	const unsigned long long_delay_value = 2147483640;
//	for (counter = 0;counter < long_delay_value;counter++)
//	{
//
//	}
/************************************************************/
//#endif /* BOARD_VNS_6_REF */

	switch(data->cmd){
            case INIT_CMD_INIT:
                // Initialize and register trace resources
                //i2c_busy = FALSE;
                debug_test_mode = FALSE;
                init_led_values();
                enable_i2c();
                VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
                VTSS_TRACE_REGISTER(&trace_reg);
                vns_status = VNS_STATE_IDLE;
#ifdef VTSS_SW_OPTION_VCLI
                vns_fpga_cli_init();
#endif
                /* StartLED(); */
                critd_init(&vns_global.registermutex, "vns_core", VTSS_MODULE_ID_VNS_FPGA, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
                FPGA_REG_UNLOCK();
                vns_global.init_complete = 0;
                // vns_fpga_conf_read(VTSS_ISID_GLOBAL,0);
                break;
            case INIT_CMD_CONF_DEF:
                //		vns_global.fpga_update.disable_timer = 1;
                //		VTSS_OS_MSLEEP(70); /*Make sure timer is has time */

                /* cancel_timer(); */
                if (isid == VTSS_ISID_LOCAL) {
                    /* Reset local configuration */
                    vns_fpga_conf_read(isid,1);
                } else if (isid == VTSS_ISID_GLOBAL) {
                    /* Reset configuration to default (local config on specific switch or global config for whole stack) */
                    vns_fpga_conf_read(isid,1);
                }
                if(set_vns_config())
                {
                    T_E("Failed to set VNS config");
                }
                //		vns_global.fpga_update.disable_timer = 0;
                /* init_timer(); */
                break;
            case INIT_CMD_MASTER_UP:
                /* cyg_thread_resume(LED_thread_handle); */
                //		vns_fpga_conf_read(VTSS_ISID_GLOBAL,0); cyg_i2c_transaction_end
                //        if(set_vns_config())
                //        {
                //        	T_E("Failed to set VNS config");
                //        }
                //init_timer();
                //		if(Start_TimeInput())
                //		{
                //			T_W("VNS FPGA: Failed to start time input!\n");
                //		}
                //		if(Start_TimeOutput())
                //		{
                //			T_W("VNS FPGA: Failed to start time output!\n");
                //		}
                break;
            case INIT_CMD_START:
                T_D("INIT_CMD_START");
                /*
                 * The code from INIT_CMD_START has been moved to init_setup_timer.
                 * A longer delay was needed on a hard reset for, the PTP module was not ready to be configured.
                 *
                 */
                init_setup_timer();
#ifdef IES_DEBUGGING
                init_debug_timer();
#endif
                break;
            case INIT_CMD_MASTER_DOWN:

                break;
            case INIT_CMD_SWITCH_ADD:

                break;
            case INIT_CMD_SWITCH_DEL:

                break;
            case INIT_CMD_SUSPEND_RESUME:

                break;
            case INIT_CMD_WARMSTART_QUERY:

                break;
            default:
                break;
	}

	return VTSS_RC_OK;
}
// **** START Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////

int GetSystemHealthReg(uint32_t * sys_health)
{
	*sys_health = 0;
	return 0;//GetRegisterWord(FPGA_REG_FPGA_SYS_HEALTH, sys_health);
}
int GetDiscreteInHealthReg(uint32_t *di_health)
{
	*di_health = 0;
	return 0;
}
int GetDiscreteOutHealthReg(uint32_t *do_health)
{
	*do_health = 0;
	return 0;
}

int GetGPSHealthReg(uint32_t *gps_health)
{
	*gps_health = 0;
	vns_time_input time_in;
	if(Get_TimeInConfig(&time_in))
	{
		return 1;
	}
	if(time_in.source == TIME_INPUT_SRC_GPS0 ||
			time_in.source == TIME_INPUT_SRC_GPS3 ||
			time_in.source == TIME_INPUT_SRC_GPS5)
	{
		return GetRegister(FPGA_IRIG_IN_HEALTH, gps_health);
	}
	return 0;
}
int GetTimeInHealth(uint32_t *time_in_health)
{
	return GetTimeInHealthReg(time_in_health);
}
/* Try to use GetTimeInHealth.  Not all health is located in the FPGA Register */
int GetTimeInHealthReg(uint32_t *time_in_health)
{
	vns_time_input_src_t src;
	Get_TimeInSource(&src);
	if(src == TIME_INPUT_SRC_1588) {
		if(ptp_lock == FALSE) {
			*time_in_health = TI_HEALTH_NO_SIGNAL;
		}
		else {
			*time_in_health = 0;
		}
		return 0;
	}
	return GetRegister(FPGA_IRIG_IN_HEALTH, time_in_health);
}
int GetTimeOutHealth(uint32_t *time_out_health)
{
	return GetTimeOutHealthReg(time_out_health);
}
int GetTimeOutHealthReg(uint32_t *time_out_health)
{
	return GetRegister(FPGA_IRIG_OUT_HEALTH, time_out_health);
}
int GetPercentComplete(int * percent_complete)
{
	//TODO: IMPLEMENT PERCENT COMPLETE
	*percent_complete = 0;
	return 0;
}


/* Remote Update Functions START*/
int Set_UpdateFactoryImg(u16 val)
{
	int retval = 0;
	retval = SetRegister(FPGA_UPDATE_FACTORY_IMG,(u32)val);
	return retval;
}
int Get_UpdateFactoryImg(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_FACTORY_IMG, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}
int Set_UpdateLoadFactoryImg(u16 val)
{
	int retval = 0;
	retval = SetRegister(FPGA_UPDATE_LOAD_FACTORY_IMG,(u32)val);
	return retval;
}
int Get_UpdateLoadFactoryImg(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_LOAD_FACTORY_IMG, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}

int Set_UpdateChunkModeEnable(u16 val)
{
	int retval = 0;
	retval = SetRegister(FPGA_UPDATE_CHUNK_MODE_ENABLE,(u32)val);
	return retval;
}
int Get_UpdateChunkModeEnable(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_CHUNK_MODE_ENABLE, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}

int Set_UpdateData(unsigned int val)
{
	int retval = 0;
	retval = SetRegister(FPGA_UPDATE_DATA,(u32)val);
	return retval;
}
int Get_UpdateData(unsigned int * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_DATA, &reg_val);
	if(retval == 0)
	{
		*val = reg_val;
	}
	return retval;
}

int Get_UpdateStatus(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_STATUS, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}

int Get_IsUpdateFactoryImg(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_FACTORY, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}

int Get_IsUpdateDone(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_DONE, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}

int Get_IsUpdateEraseDone(u16 * val)
{
	int retval = 0;
	u32 reg_val;
	retval = GetRegister(FPGA_UPDATE_ERASE_DONE, &reg_val);
	if(retval == 0)
	{
		*val = reg_val & 0xFFFF;
	}
	return retval;
}
int UpdateVNSFPGACheckForFailure(char* info, int val, int retval)
{
	T_D("%s : %d : %d", info, val, retval);
	if(retval) {
		firmware_status_set("Failure occurred");
		T_D("Failure occurred");
		return -1;
	}
	return 0;
}
unsigned int calculate_percent(unsigned int val, unsigned int total)
{
	double d_val = val;
	return (d_val * 100)/total;
}
/*
 * File size must be divisible by 4
 */
void* UpdateVNSFPGAFirmware(void* update_args  )
{
	u32 i ,
		retval = 0,
		temp = 0,
		file_size = 0,
		checksum = 0;
	u32* file_data_ptr ;
	u16 val;
	unsigned char chk_sum = 0;
	const size_t update_status_str_size = 100;
	char update_status[update_status_str_size];
	update_args_t *args = update_args;



	/* get copy of file and pad it on 4 byte boundry if needed */
	if(args->file_length % (sizeof(int))) {
		file_size = args->file_length + sizeof(int) - args->file_length%(sizeof(int));
	}
	else {
		file_size = args->file_length;
	}
	T_D("Malloc");
	if(!(file_data_ptr = calloc(1,file_size ))) {
		char * err = "Allocation of firmware buffer failed";
		T_E("%s (len %zu)", err, file_size);
	}

	memcpy(file_data_ptr, args->buffer, args->file_length);
	calculate_checksum32_buffer(&checksum, args->buffer , args->file_length);
	T_D("buffer %X, file_size %u, cksm %X",(unsigned int)args->buffer, (unsigned int)args->file_length,(unsigned int)checksum );

	checksum = 0;
	calculate_checksum32_buffer(&checksum,file_data_ptr , file_size);
	T_D("file_ptr %X, file_size %u, cksm %X",(unsigned int)file_data_ptr,(unsigned int) file_size,(unsigned int)checksum );



	T_D("Chunking file over...file_length %u",file_size);

	fpga_firmware_status_set("Configuring FPGA to receive update");
	T_D("%s", fpga_firmware_status_get());

	clear_fpga_update_mode();
	/* Trigger update mode */
	retval += Get_UpdateFactoryImg(&val);

	if(UpdateVNSFPGACheckForFailure("Get_UpdateFactoryImg", val, retval)) { return NULL;}
	retval = Set_UpdateFactoryImg(1);
	retval = Get_UpdateFactoryImg(&val);
	if(UpdateVNSFPGACheckForFailure("Get_UpdateFactoryImg", val, retval)) { return NULL;}


	cyg_thread_delay(CYG_MSEC2TICK(1)); /* delay 1 millisecond per document */

	retval = Set_UpdateFactoryImg(0);
	retval = Get_UpdateFactoryImg(&val);
	if(UpdateVNSFPGACheckForFailure("Get_UpdateFactoryImg", val, retval)) { return NULL;}


	/* Wait until FPGA erases flash */
	T_D("Waiting for FPGA to erase flash...");
	fpga_firmware_status_set("Waiting for FPGA to erase flash...");
	T_D("%s", fpga_firmware_status_get());
	val = 0;
	while(!val)
	{
		retval = Get_IsUpdateEraseDone(&val);
		if(UpdateVNSFPGACheckForFailure("Get_IsUpdateEraseDone", val, retval)) { return NULL;}
		cyg_thread_delay(CYG_MSEC2TICK(3000));
	}

	/* Chunk over file */
	T_D("Chunking file over...file_length %u",file_size);
	memset(update_status, 0, update_status_str_size);
	sprintf(update_status, "Transfering image to FPGA, file_length %u",file_size);
	fpga_firmware_status_set(update_status );
	for(i=0; i < (file_size>>2); i++)
	{
		if((i & 0xFF) == 0) {
			if(temp != file_data_ptr[i]) {
				temp = file_data_ptr[i];
				T_D("%08X: %08X\n", i, file_data_ptr[i]);
				memset(update_status, 0, update_status_str_size);
				sprintf(update_status, "Transfer Progress: %% %u",calculate_percent(i, (file_size>>2)));
				fpga_firmware_status_set(update_status );
				T_D("%s", fpga_firmware_status_get());
			}
		}


		chk_sum += file_data_ptr[i];
		Set_UpdateData(file_data_ptr[i]);
	}



	T_D("Chunking file Done...");
//W vns_fpga 00:30:57 62/vns_fpga_timer_cb#2300: Warning: VNS_FGA: Failed to get ptp status
	T_D("Waiting for FPGA to finish update...");
	fpga_firmware_status_set("Waiting for FPGA to finish update...");
	T_D("%s", fpga_firmware_status_get());
//	val = 0;
//	while(!val)
//	{
//		retval = Get_IsUpdateDone(&val);
//		if(UpdateVNSFPGACheckForFailure("Get_IsUpdateDone", val, retval)) { return NULL;}
//		cyg_thread_delay(CYG_MSEC2TICK(3000));
//	}

//	u16 is_facory_img;
//	retval = Get_IsUpdateFactoryImg(&is_facory_img);
//	if(UpdateVNSFPGACheckForFailure("Get_UpdateFactoryImg", is_facory_img, retval)) { return NULL;}
//	if(is_facory_img) {
//		fpga_firmware_status_set("failure");
//		T_D("FPGA still has the factory image...");
//	}
//	else {
//		fpga_firmware_status_set(NULL);
//	}
	fpga_firmware_status_set(NULL);
	T_D("%s", fpga_firmware_status_get());
	if(NULL != file_data_ptr) {
		T_D("Free!!!");
		free(file_data_ptr);
		file_data_ptr = NULL;
	}
	return (void*)retval;
}
const char *fpga_firmware_status_get(void)
{
    return fpga_firmware_status;
}

void fpga_firmware_status_set(const char *status)
{
    fpga_firmware_status = status ? status : "idle";
    T_I("Firmware update status: %s", fpga_firmware_status);
}
char calc_checksum(void* file_ptr, u32 file_size)
{
	char checksum = 0;
	char* char_ptr = file_ptr;
	u32 i;
	for(i=0; i < file_size; i++)
		checksum += char_ptr[i];
	return checksum;
}
void clear_fpga_update_mode()
{
	VTSS_ECOS_MUTEX_LOCK(&(vns_global.fpga_update.mutex));
	vns_global.fpga_update.is_duplicating = 0;
	VTSS_ECOS_MUTEX_UNLOCK(&(vns_global.fpga_update.mutex));
}
void set_fpga_update_mode(update_args_t* args)
{
	VTSS_ECOS_MUTEX_LOCK(&(vns_global.fpga_update.mutex));
	vns_global.fpga_update.is_duplicating = 1;
	vns_global.fpga_update.must_update = 1;
	vns_global.fpga_update.disable_timer = 1;
//	vns_global.fpga_update.file_info = *args;
	vns_global.fpga_update.file_info.buffer = args->buffer;
	vns_global.fpga_update.file_info.file_length = args->file_length;
	VTSS_ECOS_MUTEX_UNLOCK(&(vns_global.fpga_update.mutex));
}
int is_fpga_update_ready()
{
	int ready = 0 ;
	VTSS_ECOS_MUTEX_LOCK(&(vns_global.fpga_update.mutex));
	if(!vns_global.fpga_update.is_duplicating)
		ready = 1;
	VTSS_ECOS_MUTEX_UNLOCK(&(vns_global.fpga_update.mutex));
	return ready;
}
#define MSB2LSBW( x )   ( x << 8 | (char)x )

#define MSB2LSBDW( x )  (\
              ( ( x & 0x000000FF ) << 24 ) \
            | ( ( x & 0x0000FF00 ) << 8 ) \
            | ( ( x & 0x00FF0000 ) >> 8 ) \
            | ( ( x & 0xFF000000 ) >> 24 ) \
             )


//#define FPGA_UPDATE_HEADER_SIGNATURE	0x69455358
#define FPGA_UPDATE_HEADER_SIZE			24
typedef enum{
	UPDATE_FILE_SIGNATURE = 0,
	UPDATE_FILE_VERSION,
	UPDATE_FILE_FILE_LENGTH,
	UPDATE_FILE_HEADER_LENGTH,
	UPDATE_FILE_FILE_CHECKSUM,
	UPDATE_FILE_HEADER_CHECKSUM,
	UPDATE_FILE_HEADER_END
} header_components_t;

typedef struct fpga_update_file_header {
	unsigned int element[UPDATE_FILE_HEADER_END];
}fpga_update_file_header_t;

typedef struct fpga_update_file_element_string {
	char *name;

}fpga_update_file_element_string_t;
const fpga_update_file_element_string_t fpga_update_file_element_string[] = {
    [UPDATE_FILE_SIGNATURE]       = {"SIGNATURE"},	// 0
    [UPDATE_FILE_VERSION]         = {"VERSION"},
    [UPDATE_FILE_FILE_LENGTH]     = {"FILE LENGTH"},
    [UPDATE_FILE_HEADER_LENGTH]   = {"HEADER LENGTH"},
    [UPDATE_FILE_FILE_CHECKSUM]   = {"FILE CHECKSUM"},
    [UPDATE_FILE_HEADER_CHECKSUM] = {"HEADER CHECKSUM"},
    [UPDATE_FILE_HEADER_END]      = {""}
};

int calculate_checksum32_buffer(unsigned int* checksum,  void * file_ptr, size_t file_length)
{
	unsigned char pack_size = 0;
	char *char_tptr ;
	void *deploy_ptr;
	unsigned int* int_ptr,
					file_size = file_length ,
					packed_file_size = file_length,
					i = 0,
					chck_sum = 0;

	if(file_size% (sizeof(int)) != 0) {
		pack_size =  sizeof(int ) - file_size% (sizeof(int));
		packed_file_size = (file_size + pack_size);
		 T_D("Malloc!!");
		deploy_ptr = calloc(1,packed_file_size);
		if(NULL == deploy_ptr) { return -1;}
		/* Fill buffer with file contents */
		memcpy(deploy_ptr, file_ptr, file_length);
	}
	else {
		deploy_ptr = file_ptr;
	}

	char_tptr = (char*)deploy_ptr;
	int_ptr = (unsigned int*)deploy_ptr;


	for(i=0; i < (packed_file_size >>2); i++)
	{
		chck_sum +=	int_ptr[i];
		if(i < 10)
			T_D("%X ptr: %X |check: %X\n", i<<2, int_ptr[i], chck_sum);
		if(i >  (packed_file_size >>2) -10)
			T_D("%X ptr: %X |check: %X\n", i<<2, int_ptr[i], chck_sum);

	}
	*checksum = chck_sum;
	/* do not free memory that is not yours!! */
	if(deploy_ptr != file_ptr) {
		if(NULL != deploy_ptr) { T_D("FREE!!");free(deploy_ptr); deploy_ptr = NULL;}
	}

	return 0;
}

char* set_fpga_update_invalid_string(const char* msg)
{
    char* dest = NULL;
    if(msg != NULL)
    {
        dest= strcpy(vns_global.fpga_update_failure_string, msg);
    }
    return dest;
}

char* get_fpga_update_invalid_string(void)
{
    return vns_global.fpga_update_failure_string;
}

const ies_board_id_info_t ies_board_info_set[] = {
    /* [ IES_BOARDID_GENERATION ]  = { 0x0000000F,  0, "GENERATION" }, */
    /* [ IES_BOARDID_FEATURE_SET ] = { 0x000F0000, 16, "FEATURE SET"}, */
    [ IES_BOARDID_POWER_BRD_REV ]        = { 0x0000000F, 0, "POWER BOARD REVISION" },
    [ IES_BOARDID_FPGA_BRD_REV ]         = { 0x000000F0, 4, "FPGA BOARD REVISION"},
    [ IES_BOARDID_SWITCH_BRD_REV ]       = { 0x00000F00, 8, "SWITCH BOARD REVISION"},
    [ IES_BOARDID_INTERCONNECT_BRD_REV ] = { 0x0000F000, 12, "INTERCONNECT BOARD REVISION"},
    /*add above here*/
    [ IES_BOARDID_END ]         = { 0x00,  0, "END" }
};

const char* fpga_update_invalid_string [] = {
    [ FPGA_UPDATE_INVALID_N0 ] = "Valid",
    [ FPGA_UPDATE_INVALID_N1 ] = "File length was too small",
    [ FPGA_UPDATE_INVALID_N2 ] = "Header checksums do not match",
    [ FPGA_UPDATE_INVALID_N3 ] = "File checksums do not match",
    [ FPGA_UPDATE_INVALID_N4 ] = "File is for an iES-6",
    [ FPGA_UPDATE_INVALID_N5 ] = "File is for an iES-12 with an FPGA board REV 1-3",
    [ FPGA_UPDATE_INVALID_N6 ] = "File is for an iES-12 REV4 FPGA board",
    [ FPGA_UPDATE_INVALID_N7 ] = "Unknown file signature",
    [ FPGA_UPDATE_INVALID_N8 ] = "FPGA version does not support remote update.",
    [ FPGA_UPDATE_INVALID_N9 ] = "Error occurred getting FPGA version."
};

int set_board_id(u32 id)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        conf.board_id = id;
        err = conf_mgmt_board_set(&conf);
        if(err)
        { T_E("could not set board config"); }
        
    }
    return err;
}
int get_board_id(u32* id)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        *id = conf.board_id;
    }
    return err;
}

int set_board_revision(flash_conf_tags_t board, uint rev)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        conf.ies_board[board] = rev;
        /* T_D("Setting %s to %d", ies_board_info_set[board].name,rev); */
        /* conf.board_id = conf.board_id & ~ies_board_info_set[board].mask; */
        /* conf.board_id |= ( rev << ies_board_info_set[board].shift ) */
        /*     &ies_board_info_set[board].mask; */
        /* T_D("Board id is now 0x%08X", conf.board_id); */
        err = conf_mgmt_board_set(&conf);
        if(err)
        { T_E("could not set board config"); }
    }
    return err;
}

int get_board_revision(flash_conf_tags_t board, uint* rev)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        *rev = conf.ies_board[board];
        /* *rev = ( conf.board_id & ies_board_info_set[board].mask )>> ies_board_info_set[board].shift ; */
    }
    return err;
}
int get_board_revision_info(flash_conf_tags_t board, ies_board_rev_info_t* info)
{
    int err;
    conf_board_t conf;
    if(info == NULL) return -1;

    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        info->revision = ( conf.board_id & ies_board_info_set[board].mask )>> ies_board_info_set[board].shift ;
        info->name = ies_board_info_set[board].name;
    }
    return err;
}
fpga_update_invalid_type_t is_fpga_header_match(unsigned int file_signature)
{
    fpga_update_invalid_type_t failure_result=FPGA_UPDATE_INVALID_N0;
    u8 fpga_version;
    /* we need fpga version and software version to determine if we have the wrong fpga load */
    Get_FPGAFirmwareVersionMajor(&fpga_version);
    switch (file_signature) {
        case VNS_FPGA_UPDATE_HEADER_SIG_IES06_V1:
            if(!is_mac_ies6() || fpga_version != 1)
            { failure_result=FPGA_UPDATE_INVALID_N4; }
            break;
        case VNS_FPGA_UPDATE_HEADER_SIG_IES12_V1:
            if(!is_mac_ies12() || fpga_version > 2)
            { failure_result=FPGA_UPDATE_INVALID_N5; }
            break;
        case VNS_FPGA_UPDATE_HEADER_SIG_IES12_V2:
            /* if we have fpga version 3 we know we have a iES-12 */
            if( /* !is_mac_ies12() || */ fpga_version != 3)
            { failure_result=FPGA_UPDATE_INVALID_N6; }
            break;
        default:
            failure_result=FPGA_UPDATE_INVALID_N7;
    }
   

    return (failure_result);
}

int fpga_firmware_check( update_args_t *args )
{
    fpga_update_file_header_t header;
    int failure_result;
    set_fpga_update_invalid_string(NULL);
    unsigned int *int_ptr = (unsigned int*)args->buffer,
                 i = 0,
                 chk_sum = 0;
    u8 fw_version_major = 0, fw_version_minor = 0;

    if(0 == Get_FPGAFirmwareVersionMinor(&fw_version_minor) && 0 == Get_FPGAFirmwareVersionMajor(&fw_version_major) ) {
        if( 0 == fw_version_minor && 0 == fw_version_major) {
            set_fpga_update_invalid_string(
                    fpga_update_invalid_string[FPGA_UPDATE_INVALID_N8]);
            return -8;
        }
    }
    else {
            set_fpga_update_invalid_string(
                    fpga_update_invalid_string[FPGA_UPDATE_INVALID_N9]);
            return -9;
    }

    if(args->file_length < FPGA_UPDATE_HEADER_SIZE)
    {
        set_fpga_update_invalid_string(
                fpga_update_invalid_string[FPGA_UPDATE_INVALID_N1]);
        return -1;
    }
    /* read  header */
    for(i=0; i < UPDATE_FILE_HEADER_END; i++)
    {
        if(i < UPDATE_FILE_HEADER_CHECKSUM)
            chk_sum += int_ptr[i];
        header.element[i] =  MSB2LSBDW(int_ptr[i]);
        T_D("header %d, %X | ", i, header.element[i]);
    }
    T_D("\n");

    /* Verify Header checksum */
    if(header.element[UPDATE_FILE_HEADER_CHECKSUM] != chk_sum)
    {
        set_fpga_update_invalid_string(
                fpga_update_invalid_string[FPGA_UPDATE_INVALID_N2]);
        return -2;
    }

    /* Verify File checksum */
    chk_sum = 0;
    //	int_ptr = (unsigned int*)file_ptr + header.element[UPDATE_FILE_HEADER_LENGTH];

    calculate_checksum32_buffer(&chk_sum, (args->buffer + header.element[UPDATE_FILE_HEADER_LENGTH]) , header.element[UPDATE_FILE_FILE_LENGTH]);

    T_D("******************************************************************\n");
    T_D("\n%s\t\t %08X\n%s\t\t %08X\n%s\t\t %08X\n%s\t\t %08X\n%s\t\t %08X\n%s\t\t %08X\n",
            fpga_update_file_element_string[UPDATE_FILE_SIGNATURE].name, header.element[UPDATE_FILE_SIGNATURE],
            fpga_update_file_element_string[UPDATE_FILE_VERSION].name, header.element[UPDATE_FILE_VERSION],
            fpga_update_file_element_string[UPDATE_FILE_FILE_LENGTH].name, header.element[UPDATE_FILE_FILE_LENGTH],
            fpga_update_file_element_string[UPDATE_FILE_HEADER_LENGTH].name, header.element[UPDATE_FILE_HEADER_LENGTH],
            fpga_update_file_element_string[UPDATE_FILE_FILE_CHECKSUM].name, header.element[UPDATE_FILE_FILE_CHECKSUM],
            fpga_update_file_element_string[UPDATE_FILE_HEADER_CHECKSUM].name, header.element[UPDATE_FILE_HEADER_CHECKSUM]
       );
    if(header.element[UPDATE_FILE_FILE_CHECKSUM] != chk_sum)
    {
        set_fpga_update_invalid_string(
                fpga_update_invalid_string[FPGA_UPDATE_INVALID_N3]);
        return -3;
    }
    /* validate signature */
    failure_result = is_fpga_header_match(header.element[UPDATE_FILE_SIGNATURE]);
    if(failure_result)
    {
        set_fpga_update_invalid_string(
                fpga_update_invalid_string[failure_result]);
        return (failure_result);
    }
   
    args->file_length =  args->file_length  - header.element[UPDATE_FILE_HEADER_LENGTH];
    args->buffer = args->buffer + header.element[UPDATE_FILE_HEADER_LENGTH];
    args->version = header.element[UPDATE_FILE_VERSION];
    return 0;
}

/* Remote Update Functions END*/


/* XML Config functions START */

BOOL get_vns_fpga_master_conf_size(config_transport_t *conf)
{
	conf->data.size = sizeof(vns_fpga_conf_t);
	if(conf->data.size == 0)
		return FALSE;
	return TRUE;
}

BOOL get_vns_fpga_master_config(const config_transport_t *conf)
{
	BOOL result = TRUE;
	size_t ptp_conf_size = sizeof(vns_fpga_conf_t);
	if(conf->data.size >= ptp_conf_size) {
		memcpy(conf->data.pointer.c, &config_shaddow, ptp_conf_size);
	}
	else {
		result = FALSE;
	}
	return result;
}

BOOL set_vns_fpga_master_config(const config_transport_t *conf)
{
	vns_fpga_conf_t *config_import = (vns_fpga_conf_t *)conf->data.pointer.v;
	set_config_shaddow(*config_import);
	if (0 != set_vns_config() ) {
		return FALSE;
	}
	if (0 != save_vns_config()) {
		return FALSE;
	}
	return TRUE;
}
/* XML Config functions END */

/* EPE, Ethernet PCM Encoder */
/*
 * Configuration function for Altera's Integer N PLL reconfig block:
 *
 * Fin-+-->M--->ChgPump/VCO--->Filter--->C--->Fout
 *     |                             |
 *     <------------N<----------------
 *
 * Fout = Fin*(M/(2NC))
 *
 * Input: int percentage        --> amount to multiply Fin by in percent
 * Return: 0 on success, 1 on failure
 *
 * Usage:
 * Fout = Fin * (percentage/100) ...(integer division)
 *
 * Valid range for percentage: 0->250.
 *      0   -> PLL stop
 *      1   -> Fin*0.01
 *      2   -> Fin*0.02
 *      50  -> Fin*0.5
 *      100 -> Fin*1
 *      200 -> Fin*2
 *      250 -> Fin*2.5
 *
 * Limitations:
 * 1) Only modifies Fin in steps of 1 percent.
 * 2) May not work if Fin is an odd number.
 *
 */
int write_pll(pll_registers_t reg, u32 data)
{
    int errors = 0;
    errors += SetRegister(FPGA_EPE_PLL_CTRL_WR, FALSE);
    errors += SetRegister(FPGA_EPE_PLL_CTRL_ADDR, reg);
    errors += SetRegister(FPGA_EPE_PLL_WR_DATA, data);
    errors += SetRegister(FPGA_EPE_PLL_CTRL_WR, TRUE);
    errors += SetRegister(FPGA_EPE_PLL_CTRL_WR, FALSE);
    return errors;
}
int read_pll(pll_registers_t reg, uint32_t* data)
{
    int errors = 0;
    errors += SetRegister(FPGA_EPE_PLL_CTRL_ADDR, reg);
    errors += SetRegister(FPGA_EPE_PLL_CTRL_RD, TRUE);
    errors += SetRegister(FPGA_EPE_PLL_CTRL_RD, FALSE);
    errors += GetRegister(FPGA_EPE_PLL_RD_DATA, data);
    return errors;
}

int _pll_set_freq(int percentage)
{
    uint32_t reg,
             data;
    int timeout;
    int errors = 0;

    T_D("Percentage %d", percentage);

    if(percentage < VNS_EPE_PERCENT_RATE_MIN || percentage > VNS_EPE_PERCENT_RATE_MAX) {
        /* error. Outside of 0% to 200% Not supported. */
        return 1;
    }

    /* we will poll the status register to know when reconfig is done */
    write_pll(PLL_REG_MODE, PLL_POLLING_MODE);

    /* set N counter to be freq_in_mhz */
    reg = 0;
    reg = SETMSK32(reg, PLL_N_LO_MSK, 50);
    reg = SETMSK32(reg, PLL_N_HI_MSK, 50);
    reg = SETMSK32(reg, PLL_N_ODDDIV, 0);
    reg = SETMSK32(reg, PLL_N_BYPASS, 0);
    errors = write_pll(PLL_REG_N, reg);
    errors = read_pll(PLL_REG_N, &data);

    /* set M counter */
    reg = 0;
    reg = SETMSK32(reg, PLL_M_LO_MSK, percentage);
    reg = SETMSK32(reg, PLL_M_HI_MSK, percentage);
    reg = SETMSK32(reg, PLL_M_BYPASS, 0);
    reg = SETMSK32(reg, PLL_M_ODDDIV, 0);
    errors = write_pll(PLL_REG_M, reg);
    errors = read_pll(PLL_REG_M, &data);

    /* set the post scale counter to 1 */
    reg = 0;
    reg = SETMSK32(reg, PLL_C_LO_MSK, 1);
    reg = SETMSK32(reg, PLL_C_HI_MSK, 1);
    reg = SETMSK32(reg, PLL_C_BYPASS, 0);
    reg = SETMSK32(reg, PLL_C_ODDDIV, 0);
    errors = write_pll(PLL_REG_C, reg);
    errors = read_pll(PLL_REG_C, &data);

    /* PLL is not accurate if bandwidth register is not set up correctly */
    reg = 0;
    reg = SETMSK32(reg, PLL_BANDWIDTH_MSK, pll_get_bandwidth(percentage));
    errors = write_pll(PLL_REG_BANDWITH, reg);
    errors = read_pll(PLL_REG_BANDWITH, &data);

    /* PLL is not accurate if charge pump register is not set up correctly */
    reg = 0;
    reg = SETMSK32(reg, PLL_CHG_PUMP_MSK, pll_get_charge_pump(percentage));
    errors = write_pll(PLL_REG_CHG_PUMP, reg);
    errors = read_pll(PLL_REG_CHG_PUMP, &data);

    /* start the reconfig */
    errors = write_pll(PLL_REG_START, 1);

    /* wait for reconfig to finish, timeout if it takes too long */
    /* for(timeout = 100; --timeout && !read_pll(PLL_REG_STATUS);) { */
    /*     for(int i = 0; i < 100; i++) { __asm__ __volatile__ ("nop"); } */
    /* } */
    timeout = 100;
    do {
	VTSS_OS_MSLEEP(10); 
        errors = read_pll(PLL_REG_STATUS, &data);
        if(data) {
            break;
        }

    } while(--timeout > 0);

    return errors;
}

int pll_set_freq(int percentage)
{
    int try=3, errors;
    do {
        errors=_pll_set_freq(percentage);
       if(errors) {
           T_D("Trying again to configure, %d tries left", try);
       }
       else {
           break;
       }

    } while (--try > 0 );
    return errors;
}

int encoder_config_register_const(void)
{
    int errors = 0;
    T_D("Common Register sets");
    /* errors += SetRegister(FPGA_EPE_CH7_FRAME_SIZE, CH7_FRAME_SIZE_DEFAULT); */
    errors += SetRegister(FPGA_EPE_HDLC_SEND_ADDR, HDLC_SEND_ADDR_CTRL_DEFAULT);
    errors += SetRegister(FPGA_EPE_HDLC_ADDR_BYTE, HDLC_ADDR_BYTE_DEFAULT);
    errors += SetRegister(FPGA_EPE_HDLC_CTRL_BYTE, HDLC_CTRL_BYTE_DEFAULT);
    return errors;
}


int decoder_config_disable(void)
{
    int errors = 0;
    errors += encoder_config_register_const();
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_FRM_SZ, CH7_FRAME_SIZE_DEFAULT);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_HDLC_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, HDLC_CH7_MUX_DEFAULT);
    errors += write_pll(PLL_REG_START, 0);
    if(!errors)
    {
        config_shaddow.epe_config.mode = VNS_EPE_DISABLE;
    }
    return errors;
}
int encoder_config_disable(void)
{
    int errors = 0;
    errors += encoder_config_register_const();
    errors += SetRegister(FPGA_EPE_CH7_FRAME_SIZE, CH7_FRAME_SIZE_DEFAULT);
    errors += SetRegister(FPGA_EPE_CH7_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_HDLC_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, HDLC_CH7_MUX_DEFAULT);
    errors += write_pll(PLL_REG_START, 0);
    if(!errors)
    {
        config_shaddow.epe_config.mode = VNS_EPE_DISABLE;
    }
    return errors;
}

int epe_config_disable_all(void)
{
    int errors = 0;
    errors += decoder_config_disable();
    errors += encoder_config_disable();

    return errors;
}
int decoder_config_hdlc(const vns_epe_conf_blk_t* epe_conf)
{
    int errors = 0;
    /* int epe_percent = 50; */
    T_D("epe_conf->mode %u", epe_conf->mode);
    T_D("epe_conf->bit_rate %u", epe_conf->bit_rate);
    T_D("epe_conf->ch7_data_word_count %u", epe_conf->ch7_data_word_count);
    /* errors += pll_set_freq(epe_conf->bit_rate); */
    /* T_D("errors 0x%X", errors); */

    errors += encoder_config_register_const();
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_HDLC_ENABLE, 1);
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_ENABLE, 0);
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, HDLC_MUX_ENABLE);
    T_D("errors 0x%X", errors);
    if(!errors)
    {
        config_shaddow.epe_config.mode = VNS_EPE_DECODE_HDLC;
    }
    return errors;
}

int encoder_config_hdlc(const vns_epe_conf_blk_t* epe_conf)
{
    int errors = 0;
    /* int epe_percent = 50; */
    T_D("epe_conf->mode %u", epe_conf->mode);
    T_D("epe_conf->bit_rate %u", epe_conf->bit_rate);
    T_D("epe_conf->ch7_data_word_count %u", epe_conf->ch7_data_word_count);
    errors += pll_set_freq(epe_conf->bit_rate);
    T_D("errors 0x%X", errors);

    errors += encoder_config_register_const();
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_CH7_ENABLE, -1);
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_HDLC_ENABLE, -1);
    T_D("errors 0x%X", errors);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, HDLC_MUX_ENABLE);
    T_D("errors 0x%X", errors);
    if(!errors)
    {
        config_shaddow.epe_config.bit_rate = epe_conf->bit_rate;
        config_shaddow.epe_config.mode = VNS_EPE_HDLC;
    }
    return errors;
}

int decoder_config_ch7_15(const vns_epe_conf_blk_t* epe_conf)
{
    int errors = 0;
    /* int epe_percent = 50; */
    /* errors += pll_set_freq(epe_conf->bit_rate); */


    errors += encoder_config_register_const();
    errors += SetRegister(FPGA_EPE_CH7_FRAME_SIZE, epe_conf->ch7_data_word_count);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_FRM_SZ, epe_conf->ch7_data_word_count);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_ENABLE, 1);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_HDLC_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, CH7_MUX_ENABLE);
    if(!errors)
    {
        config_shaddow.epe_config.ch7_data_word_count = epe_conf->ch7_data_word_count;
        config_shaddow.epe_config.mode = VNS_EPE_DECODE_CH7_15;
    }
    T_D("errors 0x%X", errors);
    return errors;
}

int encoder_config_ch7_15(const vns_epe_conf_blk_t* epe_conf)
{
    int errors = 0;
    /* int epe_percent = 50; */
    errors += pll_set_freq(epe_conf->bit_rate);


    errors += encoder_config_register_const();
    errors += SetRegister(FPGA_EPE_CH7_FRAME_SIZE, epe_conf->ch7_data_word_count);
    errors += SetRegister(FPGA_EPE_CTRL_DEC_CH7_FRM_SZ, epe_conf->ch7_data_word_count);
    errors += SetRegister(FPGA_EPE_CH7_ENABLE, 1);
    errors += SetRegister(FPGA_EPE_HDLC_ENABLE, 0);
    errors += SetRegister(FPGA_EPE_HDLC_CH7_MODE_MUX, CH7_MUX_ENABLE);
    if(!errors)
    {
        config_shaddow.epe_config.bit_rate = epe_conf->bit_rate;
        config_shaddow.epe_config.ch7_data_word_count = epe_conf->ch7_data_word_count;
        config_shaddow.epe_config.mode = VNS_EPE_CH7_15;
    }
    T_D("errors 0x%X", errors);
    return errors;
}

BOOL set_vns_fpga_epe_conf_debug(const vns_epe_conf_blk_t epe_conf)
{
#ifdef IES_EPE_FEATURE_ENABLE
    int errors = 0;
    T_D("epe_conf.mode %u", epe_conf.mode);
    T_D("epe_conf.bit_rate %u", epe_conf.bit_rate);
    T_D("epe_conf.ch7_data_word_count %u", epe_conf.ch7_data_word_count);
    /* encoder_config_disable(); */
    /* TODO: May have to enable with new load */
    /* errors += encoder_config_disable(); */
    errors += epe_config_disable_all();
    switch(epe_conf.mode) {
        case VNS_EPE_DISABLE:
            /* do nothing */
            break;
        case VNS_EPE_HDLC:
        case VNS_EPE_DECODE_HDLC:
            errors += encoder_config_hdlc(&epe_conf);
            errors += decoder_config_hdlc(&epe_conf);
            break;
        case VNS_EPE_CH7_15:
        case VNS_EPE_DECODE_CH7_15:
            errors += encoder_config_ch7_15(&epe_conf);
            errors += decoder_config_ch7_15(&epe_conf);
            break;
        default:
            T_E("\n%s type\n", VNS_EPE_MODE_STR[VNS_EPE_END]);
            return FALSE;
    }
    if(errors) {
        T_E("\n Errors occured during configuration\n");
        return FALSE;
    }
    if (0 != save_vns_config()) {
        T_E("\n Save configuration failed \n");
        return FALSE;
    }
    return TRUE;
#else
    return TRUE;
#endif /* IES_EPE_FEATURE_ENABLE */
}
BOOL set_vns_fpga_epe_conf(const vns_epe_conf_blk_t epe_conf)
{
#ifdef IES_EPE_FEATURE_ENABLE
    int errors = 0;
    T_D("epe_conf.mode %u", epe_conf.mode);
    T_D("epe_conf.bit_rate %u", epe_conf.bit_rate);
    T_D("epe_conf.ch7_data_word_count %u", epe_conf.ch7_data_word_count);
    /* encoder_config_disable(); */
    /* TODO: May have to enable with new load */
    /* errors += encoder_config_disable(); */
    errors += epe_config_disable_all();
    switch(epe_conf.mode) {
        case VNS_EPE_DISABLE:
            /* do nothing */
            T_D("VNS_EPE_DISABLE 0x%X", errors);
            break;
        case VNS_EPE_HDLC:
            /* errors += decoder_config_disable(); */
            errors += encoder_config_hdlc(&epe_conf);
            break;
        case VNS_EPE_CH7_15:
            /* errors += decoder_config_disable(); */
            errors += encoder_config_ch7_15(&epe_conf);
            break;
        case VNS_EPE_DECODE_HDLC:
            /* errors += encoder_config_disable(); */
            errors += decoder_config_hdlc(&epe_conf);
            break;
        case VNS_EPE_DECODE_CH7_15:
            /* errors += encoder_config_disable(); */
            errors += decoder_config_ch7_15(&epe_conf);
            break;
        default:
            T_E("\n%s type\n", VNS_EPE_MODE_STR[VNS_EPE_END]);
            return FALSE;
    }
    if(errors) {
        T_E("\n Errors occured during configuration\n");
        return FALSE;
    }
    T_D("epe_conf.mode %u", config_shaddow.epe_config.mode );
    T_D("epe_conf.bit_rate %u", config_shaddow.epe_config.bit_rate );
    T_D("epe_conf.ch7_data_word_count %u", config_shaddow.epe_config.ch7_data_word_count );
    if (0 != save_vns_config()) {
        T_E("\n Save configuration failed \n");
        return FALSE;
    }
    return TRUE;
#else
    return TRUE;
#endif /* IES_EPE_FEATURE_ENABLE */
}
BOOL get_vns_fpga_epe_conf(vns_epe_conf_blk_t* epe_conf)
{
#ifdef IES_EPE_FEATURE_ENABLE
    if(epe_conf == NULL) return FALSE;
    memset(epe_conf, 0 , sizeof(vns_epe_conf_blk_t));

    *epe_conf = config_shaddow.epe_config; 
    /* epe_conf->mode                       = config_shaddow.epe_config.mode; */
    /* epe_conf->bit_rate                   = config_shaddow.epe_config.bit_rate; */
    /* epe_conf->ch7_data_word_count        = config_shaddow.epe_config.ch7_data_word_count; */

    T_D("epe_conf->.mode %u", epe_conf->mode);
    T_D("epe_conf->.bit_rate %u", epe_conf->bit_rate);
    T_D("epe_conf->.ch7_data_word_count %u", epe_conf->ch7_data_word_count);

    return TRUE;
#else
    return TRUE;
#endif /* IES_EPE_FEATURE_ENABLE */
}
/* EPE, Ethernet PCM Encoder */

BOOL is_epe_license_valid(vns_fpga_epe_license_info_t* license_info)
{
    ulong mac, 
          crypt_seed ;
    conf_board_t conf;
    vns_fpga_epe_license_info_t a_license_info;

    if (conf_mgmt_board_get(&conf) < 0)
        return FALSE;
    /* getting mac for seed */
    mac  = conf.mac_address[5] << 0  & 0x000000FF;
    mac |= conf.mac_address[4] << 8  & 0x0000FF00;
    mac |= conf.mac_address[3] << 16 & 0x00FF0000;
    mac |= conf.mac_address[2] << 24 & 0xFF000000;

    /*make decrypter key*/
    crypt_seed = ( mac << 0)  &0x000000FF;
    crypt_seed|= ( mac << 8)  &0x0000FF00;
    crypt_seed|= ( mac << 16) &0x00FF0000;
    crypt_seed|= ( mac << 24) &0xFF000000;
    a_license_info.epe_signature = ( crypt_seed ^ license_info->epe_signature ) &0xFFFFFFFF;
    a_license_info.ies_signature = ( crypt_seed ^ license_info->ies_signature ) &0xFFFFFFFF;
    a_license_info.key           = ( crypt_seed ^ license_info->key ) &0xFFFFFFFF;
    if( a_license_info.epe_signature     == FPGA_EPE_HEADER_SIGNATURE &&
            a_license_info.ies_signature == VNS_FPGA_UPDATE_HEADER_SIG_IES12_V2 &&
            a_license_info.key           == mac)
        return TRUE;
    /*back door*/
    if( license_info->reserved1     == ies_fpga_epe_global_license_0 &&
        license_info->reserved2     == ies_fpga_epe_global_license_1 &&
        license_info->epe_signature == ies_fpga_epe_global_license_2 &&
        license_info->ies_signature == ies_fpga_epe_global_license_3 &&
        license_info->key           == ies_fpga_epe_global_license_4 )
        return TRUE;
    return FALSE;
}

BOOL is_epe_license_acitive(void)
{
    vns_fpga_epe_license_info_t license_info;

    get_epe_license(&license_info);
    if ( is_epe_license_valid(&license_info) )
        return TRUE;
    return FALSE;
}
BOOL is_mac_default(void)
{
    conf_board_t conf;

    if (conf_mgmt_board_get(&conf) < 0)
    {
        T_E("There was a problem getting configuration");
    }
    else 
    {
        T_D("MAC %02x-%02x-%02x-%02x-%02x-%02x ",
                conf.mac_address[0],
                conf.mac_address[1],
                conf.mac_address[2],
                conf.mac_address[3],
                conf.mac_address[4],
                conf.mac_address[5]);
        if(conf.mac_address[5] == 0 && conf.mac_address[4] == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL is_epe_decoder_able(void)
{
    if( vns_global.fw_major == 3 && 
            vns_global.fw_minor >= 7 )
        return TRUE;
    return FALSE;
}

BOOL is_epe_able(void)
{
    flash_conf_tags_t board;
    uint rev[FLASH_CONF_END_TAG];

    // Firmware version 3.x is the only that supports EPE
    if( vns_global.fw_major != 3)
        return FALSE;

    for(board=0; board< FLASH_CONF_END_TAG; board++)
    {
        if( get_board_revision( board , &rev[board]) )
        {   return FALSE; }
    }
    // first EPE enabled boards
    if( rev[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250004 &&
            rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && 
            rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
            rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
            rev[FLASH_CONF_INTERCONNECT_REV_BRD_TAG] >= 4 ) {
        return TRUE;
    }
    // 1275 boards
        /*  Rev should not matter on power board 11250201*/
        /*  Rev should not matter on interconnect board 11250204*/
    if( (( rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && rev[FLASH_CONF_POWER_BRD_TAG] == 11250001 ) ||
             rev[FLASH_CONF_POWER_BRD_TAG] == 11250201 ) &&
            rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
            rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
            rev[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250204 ) {
        return TRUE;
    }
    return FALSE;
}
BOOL is_1pps_in_able(void)
{

    if( is_mac_ies12() )
    {

        flash_conf_tags_t board;
        uint rev[FLASH_CONF_END_TAG];

        for(board=0; board< FLASH_CONF_END_TAG; board++)
        {
            if( get_board_revision( board , &rev[board]) )
            {   return FALSE; }
        }
        if( rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
                rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_INTERCONNECT_REV_BRD_TAG] >= 4 ) {
            return TRUE;
        }
        // 1275 boards
        /*  Rev should not matter on power board 11250201*/
        /*  Rev should not matter on interconnect board 11250204*/
        if( (( rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && rev[FLASH_CONF_POWER_BRD_TAG] == 11250001 ) ||
                    rev[FLASH_CONF_POWER_BRD_TAG] == 11250201 ) &&
                rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
                rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250204 ) {
            return TRUE;
        }
    }
    return FALSE;
}
BOOL is_irig_in_able(void)
{
    if( is_mac_ies12() )
    {

        flash_conf_tags_t board;
        uint rev[FLASH_CONF_END_TAG];

        for(board=0; board< FLASH_CONF_END_TAG; board++)
        {
            if( get_board_revision( board , &rev[board]) )
            {   return FALSE; }
        }
        if( rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
                rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_INTERCONNECT_REV_BRD_TAG] >= 4 ) {
            return TRUE;
        }
        if( (( rev[FLASH_CONF_POWER_REV_BRD_TAG] >= 5 && rev[FLASH_CONF_POWER_BRD_TAG] == 11250001 ) ||
                    rev[FLASH_CONF_POWER_BRD_TAG] == 11250201 ) &&
                rev[FLASH_CONF_FPGA_REV_BRD_TAG] >= 4 && 
                rev[FLASH_CONF_SWITCH_REV_BRD_TAG] >= 5 && 
                rev[FLASH_CONF_INTERCONNECT_BRD_TAG] == 11250204 ) {
            return TRUE;
        }
    }
    else if( is_mac_ies6() )
    {
        return TRUE;
    }
    return FALSE;
}
BOOL is_mac_ies12(void)
{
    conf_board_t conf;

    if (conf_mgmt_board_get(&conf) < 0)
    {
        T_E("There was a problem getting configuration");
    }
    else 
    {
        if(conf.mac_address[3] == 0x11 ) {
            return TRUE;
        }
    }
    return FALSE;
}
BOOL is_mac_ies6(void)
{
    conf_board_t conf;

    if (conf_mgmt_board_get(&conf) < 0)
    {
        T_E("There was a problem getting configuration");
    }
    else 
    {
        if(conf.mac_address[3] == 0x12 ) {
            return TRUE;
        }
    }
    return FALSE;
}
void enable_i2c(void)
{
    vns_global.disable_i2c = 0;

}
void disable_i2c(void)
{
    vns_global.disable_i2c = 1;
}
int is_i2c_disabled(void)
{
    return vns_global.disable_i2c;
}
int set_epe_license(vns_fpga_epe_license_info_t license_info)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        conf.ies_board[FLASH_CONF_EPE_LIC_0_TAG] = license_info.reserved1;
        conf.ies_board[FLASH_CONF_EPE_LIC_1_TAG] = license_info.reserved2;
        conf.ies_board[FLASH_CONF_EPE_LIC_2_TAG] = license_info.epe_signature;
        conf.ies_board[FLASH_CONF_EPE_LIC_3_TAG] = license_info.ies_signature;
        conf.ies_board[FLASH_CONF_EPE_LIC_4_TAG] = license_info.key;
        err = conf_mgmt_board_set(&conf);
        if(err)
        { T_E("could not set board config"); }
    }
    return err;
}
int get_epe_license(vns_fpga_epe_license_info_t* license_info)
{
    int err;
    conf_board_t conf;
    err = conf_mgmt_board_get(&conf);
    if(err)
    { T_E("could not get board config"); }
    else
    {
        license_info->reserved1     = conf.ies_board[FLASH_CONF_EPE_LIC_0_TAG];
        license_info->reserved2     = conf.ies_board[FLASH_CONF_EPE_LIC_1_TAG];
        license_info->epe_signature = conf.ies_board[FLASH_CONF_EPE_LIC_2_TAG];
        license_info->ies_signature = conf.ies_board[FLASH_CONF_EPE_LIC_3_TAG];
        license_info->key           = conf.ies_board[FLASH_CONF_EPE_LIC_4_TAG];
    }
    /**license_info = config_shaddow.epe_config.license_info;*/
    return err;
}

/****************************************************************************/
// VNS_FPGA error text
/****************************************************************************/
#if false
char *vns_fpga_error_txt(vtss_rc rc)
{
    char *txt;

    switch (rc) {
    case VNS_FPGA_ERROR_GEN:
        txt = "VNS_FPGA generic error";
        break;
    case VNS_FPGA_ERROR_PARM:
        txt = "VNS_FPGA parameter error";
        break;
    case VNS_FPGA_ERROR_ENTRY_NOT_FOUND:
        txt = "Entry not found";
        break;
    case VNS_FPGA_ERROR_VNS_FPGA_TABLE_FULL:
        txt = "VNS_FPGA table full";
        break;
    case VNS_FPGA_ERROR_VNS_FPGA_TABLE_EMPTY:
        txt = "VNS_FPGA table empty";
        break;
    case VNS_FPGA_ERROR_STACK_STATE:
        txt = "Invalid stack state";
        break;
    case VNS_FPGA_ERROR_UNSUPPORTED:
        txt = "Unsupported feature";
        break;
    default:
        txt = "VNS_FPGA unknown error";
        break;
    }
    return txt;
}
#endif
// **** END Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////
