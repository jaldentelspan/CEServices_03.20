/*
 * mitr_fpga_cli.c
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */
#include "vtss_api.h"
#include "cli_api.h"        /* For cli_xxx()                                           	*/
#include "cli.h"            /* For cli_req_t											*/
#include "mitr_fpga_api.h"   /* For mitr_fpga_xxx() functions.                         	*/
#include "mitr_fpga_cli.h"   /* Check that public function decls. and defs. are in sync 	*/
#include "mitr_registers.h"

typedef struct {
	time_t sys_time;
	BOOL set;
}mitr_fpga_req_t;


/****************************************************************************************/
/* Default mitr_fpga_req init function													*/
/****************************************************************************************/
static void mitr_fpga_cli_req_default_set(cli_req_t * req)
{
	mitr_fpga_req_t *fpga_req = req->module_req;
	fpga_req->sys_time = 0;
	fpga_req->set = FALSE;

}

/****************************************************************************************/
/* Initialization													*/
/****************************************************************************************/
void mitr_fpga_cli_init(void)
{// Register the size required for fpga req. structure
	  cli_req_size_register(sizeof(mitr_fpga_req_t));
}

/****************************************************************************/
// HELPER FUNCTIONS
/***************************************************************************/

BOOL verbose;



//NOTE: MOVED REGISTER GET/SET FUNCTIONS TO mitr_fpga.c

static void PrintError(int err_code)
{
	CPRINTF("E %02d", err_code);
	if(verbose){
		if(err_code > 0 && err_code < 6)
			CPRINTF("\t%s\n", MITR_ERROR_CODE_STR[err_code]);
	}
	else{
		CPRINTF("\n");
	}
}


/****************************************************************************/
// Command invokation functions
/****************************************************************************/


/****************************************************************************/
// FPGA_i2c_debug_write()
/****************************************************************************/


static void FPGA_dot_help_cmd(cli_req_t * req)
{
//	CPRINTF(".1588 [master|slave|disable]\n");
	CPRINTF(".HELP\n");
	CPRINTF(".PORTSTATE\n");
	CPRINTF(".TIME [set-time]\n");
	CPRINTF(".VERSION\n");
}

static void FPGA_dot_portstate_cmd(cli_req_t * req)
{
	u32 link, act, speed;

	Get_PortState(&link, &act, &speed);

	cli_header("Link       Activity   Speed1G", 1);
	CPRINTF("%08X   %08X   %08X\n", link, act, speed);
}

//static void FPGA_dot_1588_cmd(cli_req_t * req)
//{
//	mitr_1588_type ptp_mode;
//	mitr_fpga_req_t *fpga_req = req->module_req;
//
//	if(Get_IEEE_1588_Mode(&ptp_mode))
//	{
//		PrintError(MITR_ERROR_COMMAND_FAILED);
//		CPRINTF("\n");
//	}
//	else
//	{
//		if(fpga_req->set)
//		{
////			if(fpga_req->ieee_1588_mode == MITR_IEEE_1588_SLAVE)
////				Set_TimeInSource(TIME_INPUT_SRC_1588);
//			if(Set_IEEE_1588_Mode(fpga_req->ieee_1588_mode))
//			{
//				CPRINTF("COMMAND FAILED\n");
//				PrintError(MITR_ERROR_COMMAND_FAILED);
//			}
//			else
//			{
//				//save_mitr_config();
//			}
//		}
//		else
//		{
//			cli_header("IEEE-1588 Configuration", 1);
//			CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[ptp_mode]);
//		}
//	}
//}

static void FPGA_dot_time_cmd(cli_req_t * req)
{
	mitr_fpga_req_t *fpga_req = req->module_req;
	time_t t;
	if(fpga_req->set)
	{
		if(SetSystemTime(fpga_req->sys_time)){
			CPRINTF("COMMAND FAILED\n");
			PrintError(MITR_ERROR_COMMAND_FAILED);
		} else {
			CPRINTF("SET TIME: %ld\n", (unsigned int)fpga_req->sys_time);
		}
	}
	else
	{
		if(GetSystemTime(&t)){
			CPRINTF("COMMAND FAILED\n");
			PrintError(MITR_ERROR_COMMAND_FAILED);
		} else {
			CPRINTF("%ld\n", (unsigned int)t);
		}
	}

}

static void FPGA_dot_version_cmd(cli_req_t * req)
{
	CPRINTF("%s v%d.%02d\n","MITR-12", MITR_VERSION_MAJOR, MITR_VERSION_MINOR);
}

/****************************************************************************/
/*  Parameter functions                                                     */
/****************************************************************************/

static int32_t cli_fpga_unix_time_parse(char *cmd, char *cmd2, char *stx,
										char *cmd_org, cli_req_t *req){
	int32_t        error;
	ulong value;
	mitr_fpga_req_t *fpga_req = req->module_req;
	error = VTSS_OK;

	fpga_req->set = TRUE;

	error = cli_parse_ulong(cmd, &value, 0, 0xFFFFFFFF);
	if(VTSS_OK == error){
		fpga_req->sys_time = (time_t)value;
	} else {
		fpga_req->set = FALSE;
		error = VTSS_INVALID_PARAMETER;
	}

	return VTSS_OK;
}
/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/

static cli_parm_t mitr_fpga_cli_parm_table[] = {
	{
		"unix-seconds",
		"\ttime to set in seconds (unix time)\n",
		CLI_PARM_FLAG_SET,
		cli_fpga_unix_time_parse,
		NULL
	},
    {
        NULL,
        NULL,
        0,
        0,
        NULL
    },
};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/

cli_cmd_tab_entry (
  NULL,
  ".PORTSTATE",
  "Returns port state info for MITR",
  CLI_CMD_SORT_KEY_DEFAULT,
  CLI_CMD_TYPE_CONF,
  VTSS_MODULE_ID_MITR_FPGA,
  FPGA_dot_portstate_cmd,
  mitr_fpga_cli_req_default_set,
  mitr_fpga_cli_parm_table,
  CLI_CMD_FLAG_NONE
);


cli_cmd_tab_entry (
  NULL,
  ".HELP",
  "Displays all dot commands.",
  CLI_CMD_SORT_KEY_DEFAULT,
  CLI_CMD_TYPE_CONF,
  VTSS_MODULE_ID_MITR_FPGA,
  FPGA_dot_help_cmd,
  mitr_fpga_cli_req_default_set,
  mitr_fpga_cli_parm_table,
  CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
  NULL,
  ".TIME [unix-seconds]",
  "Gets or sets the system time in unix-seconds.",
  CLI_CMD_SORT_KEY_DEFAULT,
  CLI_CMD_TYPE_CONF,
  VTSS_MODULE_ID_MITR_FPGA,
  FPGA_dot_time_cmd,
  mitr_fpga_cli_req_default_set,
  mitr_fpga_cli_parm_table,
  CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
  NULL,
  ".VERSION",
  "Gets the current MITR type and version.",
  CLI_CMD_SORT_KEY_DEFAULT,
  CLI_CMD_TYPE_CONF,
  VTSS_MODULE_ID_MITR_FPGA,
  FPGA_dot_version_cmd,
  mitr_fpga_cli_req_default_set,
  mitr_fpga_cli_parm_table,
  CLI_CMD_FLAG_NONE
);


