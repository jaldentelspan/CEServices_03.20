/*
 * mitr_fpga.c
 *
 *  Created on: Mar 21, 2013
 *      Author: eric
 */

//#define DEBUG

#include "vtss_api.h"
#include "time.h"
#include "mitr_fpga_api.h"
// **** to enble conf block, CONF_BLK_MITR_FPGA_CONF must be uncommented from "conf_api.h
#include "conf_api.h"
#ifdef VTSS_SW_OPTION_VCLI
#include "mitr_fpga_cli.h"
#endif
#include "misc_api.h"
#include "mitr_types.h"
#include "mitr_registers.h"
#include "vtss_timer_api.h"
#include "port_api.h"

//#ifndef DEBUG
#include "ptp_api.h"
#include "vtss_ptp_types.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_tod_api.h"
#include "mitr_fpga_ptp.h"
//#endif


/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include "vtss_module_id.h"
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>


#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MITR_FPGA
#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_IRQ     1
#define TRACE_GRP_CNT          2
static int init_complete = 0;

#ifndef NULL
#define NULL (void*)0
#endif

#define PTP_TIMEOFFSET 1

#if (VTSS_TRACE_ENABLED)
// Trace registration. Initialized by mitr_fpga_init() */
static vtss_trace_reg_t trace_reg = {
  .module_id = VTSS_TRACE_MODULE_ID,
  .name      = "mitr_fpga",
  .descr     = "MITR FPGA module"
};

#ifndef VTSS_MITR_FPGA_DEFAULT_TRACE_LVL
#define VTSS_MITR_FPGA_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
  [VTSS_TRACE_GRP_DEFAULT] = {
    .name      = "default",
    .descr     = "Default",
    .lvl       = VTSS_MITR_FPGA_DEFAULT_TRACE_LVL,
    .timestamp = 1,
  },
  [VTSS_TRACE_GRP_IRQ] = {
    .name      = "IRQ",
    .descr     = "IRQ",
    .lvl       = VTSS_MITR_FPGA_DEFAULT_TRACE_LVL,
    .timestamp = 1,
    .irq       = 1,
    .ringbuf   = 1,
  },
};
#endif /* VTSS_TRACE_ENABLED */

static u32 portLink = 0;
static u32 portAct = 0;
static u32 portSpeed = 0;

#if 0
static int _setledsanddiscretes(u32 link, u32 activity, u32 speed)
{
	int retval = 0;
	u32 ledg = 0;
	u32 ledr_b = 0;
	u32 led_flash = 0;
	u32 ledg0 = 0;
	u32 ledr_b0 = 0;
	u32 led_flash0 = 0;
	u32 doState = 0;
	u32 newDoState = 0;
	//setup port leds
	// port 1 is unused, and MITR port 1 is vsc port 2, so shift out port 1
	ledg = (link >> 1);
	ledr_b = (link >> 1) & (speed >> 1) ;
	led_flash = (link >> 1) & (activity >> 1);

	//write led info it to the fpga
	retval += GetRegister(MITR_FPGA_LED_CONTROL_R_B, &ledr_b0);
	retval += GetRegister(MITR_FPGA_LED_CONTROL_G, &ledg0);
	retval += GetRegister(MITR_FPGA_LED_CONTROL_FLASH, &led_flash0);

	retval += GetRegister(MITR_FPGA_DISC_OUT_STATE, &doState);//this is inverted
	newDoState = ~doState & ~(DISC_OUT_PORT1 | DISC_OUT_PORT2 | DISC_OUT_PORT3 | DISC_OUT_PORT4);
	newDoState |= ((led_flash & PORT_1_LED) ? DISC_OUT_PORT1 : 0);
	newDoState |= ((led_flash & PORT_2_LED) ? DISC_OUT_PORT2 : 0);
	newDoState |= ((led_flash & PORT_3_LED) ? DISC_OUT_PORT3 : 0);
	newDoState |= ((led_flash & PORT_4_LED) ? DISC_OUT_PORT4 : 0);
			//((led_flash & 0x1E) << 8); /* only the first 4 ports */

	ledr_b0 &= 0xFF000000;
	ledg0 &= 0xF0000000;
	led_flash0 &= 0xF0000000;

	if(!debug_test_mode)
	{
		retval += SetRegister(MITR_FPGA_LED_CONTROL_R_B, ledr_b | ledr_b0);
		retval += SetRegister(MITR_FPGA_LED_CONTROL_G, ledg | ledg0);
		retval += SetRegister(MITR_FPGA_LED_CONTROL_FLASH, led_flash | led_flash0);

		retval += SetRegister(MITR_FPGA_DISC_OUT_STATE, ~newDoState);
	}

	return retval;
}
#endif

void Get_PortState(u32 *link, u32 *activity, u32 *speed)
{
	// port 1 is unused, and MITR port 1 is vsc port 2, so shift out port 1
	*link = (portLink >> 1);
	*activity = (portAct >> 1) ;
	*speed = (portSpeed >> 1);
}

int Set_leds_and_discretes(u32 link, u32 activity, u32 speed)
{
	if(init_complete)
	{
		portLink = link;
		portAct = activity;
		portSpeed = speed;
		return 0;//_setledsanddiscretes(link, activity, speed);
	}
	else
	{
		return 0;
	}
}

#if 0
int status_ptp(ptp_clock_default_ds_t *clock_bs,
						vtss_ptp_ext_clock_mode_t *mode,
						vtss_timestamp_t *t,
						ptp_clock_slave_ds_t *clock_slave_bs)
{

    int retval = 0;
    uint inst;
    BOOL dataSet = FALSE;
    u32 hw_time; //not sure what this is for...

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

    return retval;
}
#endif

int GetSystemTime(time_t *t)
{
	int retval = 0;
//	if(-1 == time( t )){
//		retval++;
//	}
	vtss_timestamp_t ts;
	u32 hw_time; //not sure what this is for...
	ts.seconds = 0;
	ts.nanoseconds = 0;
	vtss_local_clock_time_get(&ts, 0, &hw_time);
	*t = ts.seconds;
	return retval;
}

int SetSystemTime(time_t t)
{
	int retval = 0;
	vtss_timestamp_t ts;
	u32 hw_time; //not sure what this is for...
	ts.seconds = 0;
	ts.nanoseconds = 0;
	vtss_local_clock_time_get(&ts, 0, &hw_time);
	if(ts.seconds != t){
		ts.seconds = t;
		vtss_local_clock_time_set(&ts, 0);
		retval = cyg_libc_time_settime( t );
	}
	return retval;
}



/****************************************************************************/
// mitr_fpga_init()
// Module initialization function.
/****************************************************************************/
vtss_rc mitr_fpga_init(vtss_init_data_t *data)
{

	switch(data->cmd){
	case INIT_CMD_INIT:
		 VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
		 VTSS_TRACE_REGISTER(&trace_reg);
#ifdef VTSS_SW_OPTION_VCLI
		 mitr_fpga_cli_init();
#endif


		break;
	case INIT_CMD_CONF_DEF:

		break;
	case INIT_CMD_MASTER_UP:

		break;
	case INIT_CMD_START:
		T_D("MITR_FPGA: INIT_CMD_START");
		init_complete = 1;
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

