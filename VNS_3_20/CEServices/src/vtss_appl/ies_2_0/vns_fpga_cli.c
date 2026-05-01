/*
 * vns_fpga_cli.c
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */
#include "vtss_api.h"
#include "cli_api.h"        /* for cli_xxx()                                           	*/
#include "cli.h"            /* for cli_req_t and cli_parse_integer		        */
#include "conf_api.h"       /* for conf_board_t			                        */
#include "vns_fpga_api.h"   /* for vns_fpga_xxx() functions.                         	*/
#include "vns_fpga_cli.h"   /* check that public function decls. and defs. are in sync 	*/
#include "vns_registers.h"
#include "vns_types.h"
#include "cli_api.h"
#include "vtss_ptp_local_clock.h"
/* #include "vns_strings.c" */

#if defined( BOARD_VNS_6_REF )
#include "vns_fpga_cli_6_port.h"
#elif defined( BOARD_VNS_8_REF )
#include "vns_fpga_cli_8_port.h"
#elif defined( BOARD_VNS_12_REF )
#include "vns_fpga_cli_12_port.h"
#elif defined( BOARD_VNS_16_REF )
#include "vns_fpga_cli_16_port.h"
#elif defined( BOARD_VNS_IGU_REF )
#include "vns_fpga_cli_igu.h"
#else
#error "Need to add help strings"
#endif

#include "mirror_api.h"
#include "msg_api.h"

#include "mirror_cli.h"

#include "system.h"
#include "tsd_tse_sxe.h"
#include "tsd_pll_reconfig.h"
#include "tsd_hdlc_ch7_encoder.h"
#include "tsd_hdlc_ch7_decoder.h"
#include "tsd_hdlc_decoder_2p0.h"
#include "tsd_hdlc_encoder_2p0.h"
#include "tsd_ch7_encoder_2p0.h"
#include "tsd_ch7_decoder_2p0.h"
#include "tsd_eth_tse_regs.h"

#include "ies_fpga_led.h"
#include "tsd_shift_reg_led_regs.h"
#include "tsd_register_common.h"
#include "tsd_shift_reg_led.h"

//#include "stdlib.h"
//#include "tsd_pll_reconfig.h"
//#include "tsd_hdlc_ch7_encoder.h"
//#include "tsd_ch7_encoder_2p0.h"
//#include "tsd_hdlc_encoder_2p0.h"

#define MAC_ADDRESS_MAX_VALUE	65536		/* 2^16 */

typedef struct {
    uchar fpga_i2c_data[256];
    uint32_t data_word;
    int fpga_i2c_data_cnt;
    int reg_addr;
    uint32_t reg_offset;
    vns_direction_t direction;
    int port_num;
    int time_seconds;
    vns_do_setting_t disc_out_setting;
    vns_di_setting_t disc_in_setting;
    vns_gps_dc_bias_t gps_bias_setting;
    vns_time_input_src_t time_in_setting;
    vns_time_output_t time_out_setting;
    vns_1588_type ieee_1588_mode;
    vns_time_input_signal_t time_in_signal;
    int disc_num;
    int feature;
    BOOL set;
    int cal_3;
    int cal_2;
    int cal_1;
    int disc_out_val;
    BOOL onoff;
    BOOL modify_clock;

    vns_fpga_epe_license_info_t license_info;
    /* ies_board_id_t board; */
    flash_conf_tags_t board;
    conf_board_t ies_board_conf;
    flash_conf_board_model_t ies_board_model;
    /* EPE Keywords */
    BOOL disable;
    BOOL encode;
    BOOL decode;
    BOOL rx;
    BOOL tx;
    vns_epe_conf_blk_t epe_config;


    u32 led_red_blue;
    u32 led_green; u32 led_flashing; 
    u32 discrete_out;
    /* To manually set time for 1PPS mode */
    vns_time_t time;
}vns_fpga_req_t;


/****************************************************************************************/
/* Default vns_fpga_req init function						        */
/****************************************************************************************/
static void vns_fpga_cli_req_default_set(cli_req_t * req)
{
    vns_fpga_req_t *fpga_req    = req->module_req;

    fpga_req->fpga_i2c_data_cnt = 0;
    fpga_req->data_word         = 0;
    fpga_req->fpga_i2c_data_cnt = 0;
    fpga_req->direction         = 0;
    fpga_req->port_num          = 0;
    fpga_req->time_seconds      = 0;
    fpga_req->disc_in_setting   = 0;
    fpga_req->disc_out_setting  = 0;
    fpga_req->gps_bias_setting  = 0;
    fpga_req->time_in_setting   = 0;
    fpga_req->time_out_setting  = 0;
    fpga_req->ieee_1588_mode    = IEEE_1588_DISABLED;
    fpga_req->time_in_signal    = TIME_INPUT_TYPE_RESERVED;
    fpga_req->feature      = -1;
    fpga_req->disc_num     = 0;
    fpga_req->set          = FALSE;
    fpga_req->cal_3        = -1;
    fpga_req->cal_2        = -1;
    fpga_req->cal_1        = -1;
    fpga_req->disc_out_val = -1;
    fpga_req->onoff        = FALSE;
    fpga_req->modify_clock = FALSE;

    memset( &fpga_req->license_info, 0, sizeof( vns_fpga_epe_license_info_t ));
    fpga_req->board           = 0;
    memset( &fpga_req->ies_board_conf, 0, sizeof( conf_board_t ));
    fpga_req->ies_board_model = 0;
    fpga_req->encode        = FALSE;
    fpga_req->decode        = FALSE;
    fpga_req->rx            = FALSE;
    fpga_req->tx            = FALSE;
    fpga_req->disable       = FALSE;
    memset( &fpga_req->epe_config, 0, sizeof( vns_epe_conf_blk_t ));
    fpga_req->epe_config.invert_clock  = FALSE;
    fpga_req->epe_config.insert_fcs  = TRUE;

    /* To manually set time for 1PPS mode */
    memset( &fpga_req->time, 0, sizeof( vns_time_t ));
}

/****************************************************************************************/
/* Initialization								        */
/****************************************************************************************/
void vns_fpga_cli_init(void)
{// Register the size required for fpga req. structure
    cli_req_size_register(sizeof(vns_fpga_req_t));
}

/****************************************************************************/
// FPGA_i2c_debug_read()
/****************************************************************************/

static void FPGA_i2c_debug_read(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    uint32_t data;
    vns_register_info reg_info;
    reg_info.mask = 0xFFFFFFFF;
    reg_info.regnum = fpga_req->reg_addr + fpga_req->reg_offset*4;
    reg_info.offset = fpga_req->reg_offset;
    reg_info.shift = 0;
    if (_GetRegister(reg_info, &data) == 0)
    {
        CPRINTF("Data read from FPGA I2C = 0x%08X, reg_addr = 0x%08X  reg_offset = 0x%08X\n",
                data,
                fpga_req->reg_addr,
                fpga_req->reg_offset
                );
    }
    else
    {
        CPRINTF("Write-Read operation failed\n");
    }
}

/****************************************************************************/
// FPGA_i2c_debug_write()
/****************************************************************************/

static void debug_time_input_reg(cli_req_t *req)
{
    debug_time_input();
}

static void debug_decoder_reg(cli_req_t *req)
{
    debug_decoder();
}

static void debug_encoder_reg(cli_req_t *req)
{
    debug_encoder();
}

static void FPGA_i2c_debug_write(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_register_info reg_info;
    uint32_t data;
    reg_info.mask = 0xFFFFFFFF;
    reg_info.regnum = fpga_req->reg_addr + fpga_req->reg_offset*4;
    reg_info.shift = 0;
    reg_info.offset = fpga_req->reg_offset;
    if(_SetRegister(reg_info, fpga_req->data_word) == 0)
    {
        CPRINTF("Data Wrote from FPGA I2C = 0x%08X, reg_addr = 0x%08X  reg_offset = 0x%08X\n",
                data,
                fpga_req->reg_addr,
                fpga_req->reg_offset
                );
    }
    else
    {
        CPRINTF("FPGA I2C Write operation failed\n");
    }
}
/****************************************************************************/
// HELPER FUNCTIONS
/***************************************************************************/

BOOL verbose;



//NOTE: MOVED REGISTER GET/SET FUNCTIONS TO vns_fpga.c

static void PrintError(int err_code)
{
    CPRINTF("E %02d", err_code);
    if(verbose){
        if(err_code > 0 && err_code < 6)
            CPRINTF("\t%s\n", VNS_ERROR_CODE_STR[err_code]);
    }
    else{
        CPRINTF("\n");
    }
}

static void PrintDiscreteInput(vns_di_setting_t disc_in)
{
    CPRINTF("%s", vns_di_setting_name[disc_in]);
}

static void PrintDiscreteOutput(vns_do_setting_t disc_out)
{
    CPRINTF("%s", vns_do_setting_name[disc_out]);
}


#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
static int GetGPSAntennaBias(vns_gps_dc_bias_t * gps_antenna_bias)
{
    vns_time_input time_in;
    if(Get_TimeInConfig(&time_in))
    {
        return 1;
    }
    if(time_in.source == TIME_INPUT_SRC_GPS3)
    {
        *gps_antenna_bias = DC_BIAS_3_3;
    }
    else if (time_in.source == TIME_INPUT_SRC_GPS5)
    {
        *gps_antenna_bias = DC_BIAS_5;
    }
    else
    {
        *gps_antenna_bias = DC_BIAS_0;
    }

    return 0;
}
#endif // defined (BOARD_VNS_12_REF)
/*
   static int GetSystemHealthReg(uint32_t * sys_health)
   {
 *sys_health = 0;
 return 0;//GetRegisterWord(FPGA_REG_FPGA_SYS_HEALTH, sys_health);
 }

 static int GetDiscreteInHealthReg(uint32_t *di_health)
 {
 *di_health = 0;
 return 0;
 }

 static int GetDiscreteOutHealthReg(uint32_t *do_health)
 {
 *do_health = 0;
 return 0;
 }

 static int GetGPSHealthReg(uint32_t *gps_health)
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

 static int GetTimeInHealthReg(uint32_t *time_in_health)
 {
 return GetRegister(FPGA_IRIG_IN_HEALTH, time_in_health);
 }

 static int GetTimeOutHealthReg(uint32_t *time_out_health)
 {
 return GetRegister(FPGA_IRIG_OUT_HEALTH, time_out_health);
 }
 static int GetPercentComplete(int * percent_complete)
 {
//TODO: IMPLEMENT PERCENT COMPLETE
 *percent_complete = 0;
 return 0;
 }

*/
/****************************************************************************/
// Command invokation functions
/****************************************************************************/


static void FPGA_dot_debug(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    FPGA_Debug_Test_Mode(fpga_req->onoff);
}

static void FPGA_dot_timer(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    BOOL disable_timer = FALSE;
    /* a little reverse logic to confuse you. CLI turns timer on and off. */
    /*     So, normal is on; function is in reverse. disable timer? */
    if(!fpga_req->onoff)
        disable_timer = TRUE;
    FPGA_Debug_disable_timer(disable_timer);
}

/****************************************************************************/
// FPGA_i2c_debug_write()
/****************************************************************************/

static void FPGA_dot_status_cmd(cli_req_t *req)
{
    vns_status_t status;
    int percent_complete;
    int warning_bits = 0; // not sure
    int error_bits = 0; // count number of health bits set (all systems, I think)
    uint32_t temp;
    int i;
    /* GET NUMBER OF ERROR BITS */
    GetSystemHealthReg(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    GetDiscreteInHealthReg(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    GetDiscreteOutHealthReg(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    GetGPSHealthReg(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    GetTimeInHealth(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    GetTimeOutHealthReg(&temp);
    for(i = 0; i < 32; i++)
    {
        if( ( (temp >> i) & 0x01) )
            error_bits++;
    }
    /* FINALLY, GET STATUS */

    status = GetStatus();
    CPRINTF("S %02d %d %d", (int)status, warning_bits, error_bits);
    switch(status)
    {
        case VNS_STATE_BIT:
            if(GetPercentComplete(&percent_complete) == 0)
            {
                CPRINTF(" %d%%", percent_complete);
            }
            break;
        case VNS_STATE_BUSY:
            if(GetPercentComplete(&percent_complete) == 0)
            {
                CPRINTF(" %d%%", percent_complete);
            }
            break;
        default:
            break;
    }
    if(verbose)
        if((status >= VNS_STATE_FAIL) && (status < VNS_STATE_UNKNOWN))
            CPRINTF("\t- %s\n", vns_status_name[status]);
        else
            CPRINTF("\n");
    else
        CPRINTF("\n");
}

static void FPGA_dot_discrete_cmd(cli_req_t *req)
{
    uint32_t di_word = 0;
    discrete_in_config di_setting;
    uint32_t do_word = 0;
    discrete_out_config do_setting;
    int i, disc_num;
    vns_fpga_req_t *fpga_req = req->module_req;

    BOOL disc_invert = TRUE;

    disc_num = fpga_req->disc_num -1;
    if(disc_num < 0)
        disc_num = 0;

    if(fpga_req->direction == DIRECTION_NONE ||
            fpga_req->direction == DIRECTION_IN )
    {
        if(Get_DiscreteInputState(&di_word))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
        if(Get_DiscreteInputConfig(&di_setting))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
    }
    if(fpga_req->direction == DIRECTION_NONE ||
            fpga_req->direction == DIRECTION_OUT )
    {
        if(Get_DiscreteOutputState(&do_word))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
        if(Get_DiscreteOutputConfig(&do_setting))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
    }

    if (fpga_req->direction == DIRECTION_IN)
    {
        if(disc_num >= DISCRETE_IN_MAX)
        {
            PrintError(VNS_ERROR_INVALID_PARAMETER);
            return;
        }
        if(fpga_req->set)
        {
            di_setting.di_setting[disc_num] = fpga_req->disc_in_setting;
            if(Set_DiscreteInputConfig(di_setting))
            {
                PrintError(VNS_ERROR_COMMAND_FAILED);
                return;
            }
            else
            {
                save_vns_config();
            }
        }
        else
        {
            if(fpga_req->disc_num == 0)
            {// print all
                for(i = 0; i < DISCRETE_IN_MAX; i++)
                {
                    if(disc_invert)
                        CPRINTF("%d: %s\t", (i+1), ( ( (di_word >> i) & 0x01) ? "Off" : "On") );
                    else
                        CPRINTF("%d: %s\t", (i+1), ( ( (di_word >> i) & 0x01) ? "On" : "Off") );
                    PrintDiscreteInput(di_setting.di_setting[i]);
                    CPRINTF("\n");
                }
            }
            else
            {
                if(disc_invert)
                    CPRINTF("%d: %s\t", (fpga_req->disc_num), ( ( (di_word >> disc_num) & 0x01) ? "Off" : "On") );
                else
                    CPRINTF("%d: %s\t", (fpga_req->disc_num), ( ( (di_word >> disc_num) & 0x01) ? "On" : "Off") );
                PrintDiscreteInput(di_setting.di_setting[disc_num]);
                CPRINTF("\n");
            }
        }
    }
    else if (fpga_req->direction == DIRECTION_OUT)
    {
        if(disc_num >= DISCRETE_OUT_MAX)
        {
            PrintError(VNS_ERROR_INVALID_PARAMETER);
            return;
        }
        if(fpga_req->set)
        {
            do_setting.do_setting[disc_num] = fpga_req->disc_out_setting;
            if(Set_DiscreteOutputConfig(do_setting))
            {
                PrintError(VNS_ERROR_COMMAND_FAILED);
                return;
            }
            else
            {
                save_vns_config();
            }
        }
        else
        {
            if(fpga_req->disc_num == 0)
            {//print all
                for(i = 0; i < DISCRETE_OUT_MAX; i++)
                {
                    if(disc_invert)
                        CPRINTF("%d: %s\t", (i+1), ( ( (do_word >> i) & 0x01) ? "Off" : "On") );
                    else
                        CPRINTF("%d: %s\t", (i+1), ( ( (do_word >> i) & 0x01) ? "On" : "Off") );
                    PrintDiscreteOutput(do_setting.do_setting[i]);
                    CPRINTF("\n");
                }
            }
            else
            {
                if(disc_invert)
                    CPRINTF("%d: %s\t", (fpga_req->disc_num), ( ( (do_word >> disc_num) & 0x01) ? "Off" : "On") );
                else
                    CPRINTF("%d: %s\t", (fpga_req->disc_num), ( ( (do_word >> disc_num) & 0x01) ? "On" : "Off") );
                PrintDiscreteOutput(do_setting.do_setting[disc_num]);
                CPRINTF("\n");
            }
        }
    }
    else //(fpga_req->direction == DIRECTION_NONE)
    {
        for(i = (DISCRETE_IN_MAX - 1); i >= 0; i--)
        {
            CPRINTF("%d", (disc_invert ? ( ~(di_word >> i) & 0x01 ) : ( (di_word >> i) & 0x01 ) ) );
        }
        CPRINTF("\t\tIn\n");
        for(i = (DISCRETE_OUT_MAX - 1); i >= 0; i--)
        {
            CPRINTF("%d", (disc_invert ? ( ~(do_word >> i) & 0x01 ) : ( (do_word >> i) & 0x01 ) ) );
        }
        CPRINTF("\tOut\n");
    }
}

static void FPGA_dot_bit_cmd(cli_req_t * req)
{
    CPRINTF("\n");
}

static void FPGA_dot_config_cmd(cli_req_t * req)
{
#if !defined(BOARD_IGU_REF)
    discrete_in_config di_setting;
    discrete_out_config do_setting;
#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
    vns_gps_dc_bias_t gps_config;
#endif // defined(BOARD_VNS_12_REF)
#endif // !defined(BOARD_IGU_REF)
    vns_time_input time_in_config;
#if !defined(BOARD_IGU_REF)
    vns_time_output time_out_config;
#endif // !defined(BOARD_IGU_REF)

    int i;

#if !defined(BOARD_IGU_REF)

    if(!Get_DiscreteInputConfig(&di_setting))
    {
        cli_header("Discrete Input Configuration", 1);
        for(i = 0; i < DISCRETE_IN_MAX; i++)
        {
            CPRINTF("%d:\t", (i+1) );
            PrintDiscreteInput(di_setting.di_setting[i]);
            CPRINTF("\n");
        }
        CPRINTF("\n");
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
    if(!Get_DiscreteOutputConfig(&do_setting))
    {
        cli_header("Discrete Output Configuration", 1);
        for(i = 0; i < DISCRETE_OUT_MAX; i++)
        {
            CPRINTF("%d:\t", (i+1) );
            PrintDiscreteOutput(do_setting.do_setting[i]);
            CPRINTF("\n");
        }
        CPRINTF("\n");
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
    if(!GetGPSAntennaBias(&gps_config))
    {
        cli_header("GPS Configuration", 1);
        CPRINTF("GPS DC Bias:\t%s\n", GPS_DC_BIAS_STR[gps_config]);
        CPRINTF("\n");
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
#endif //defined(BOARD_VNS_12_REF)

#endif // !defined(BOARD_IGU_REF)

    if(!Get_TimeInConfig(&time_in_config))
    {
        cli_header("Time Input Configuration", 1);
        CPRINTF("Time Input Source:\t%s\n", TIME_IN_SRC_STR[time_in_config.source]);
        CPRINTF("Time Input Signal:\t%s\n", TIME_IN_SIGNAL_STR[time_in_config.signal]);
        CPRINTF("\n");
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }

#if !defined(BOARD_IGU_REF)

    if(!Get_TimeOutConfig(&time_out_config))
    {
        cli_header("Time Output Configuration", 1);
        CPRINTF("Time Output Type:\t%s\n", TIME_OUT_TYPE_STR[time_out_config.timecode]);
        CPRINTF("Time Output Mode:\t%s\n", TIME_OUTPUT_MODE_STR[time_out_config.mode]);
        //CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[time_out_config.ieee_1588_type]);
        CPRINTF("\n");
        cli_header("IEEE-1588 Configuration", 1);
        CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[time_out_config.ieee_1588_type]);
        CPRINTF("\n");
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
#endif // !defined(BOARD_IGU_REF)
}

static void FPGA_dot_debug_config_cmd(cli_req_t * req)
{
    vns_fpga_conf_t * cfg = get_shadow_config();
    int i;
    if(cfg)
    {
        cli_header("Config Info", 1);
        CPRINTF("VERSION:\t%d\n", cfg->version );
        CPRINTF("VERBOSE:\t%s\n", (cfg->verbose ? "true" : "false") );

        cli_header("Time Input Configuration", 1);
        CPRINTF("Time Input CHID:\t%d\n", cfg->time_in_setting.channel_id);
        CPRINTF("Time Input Source:\t%s\n", TIME_IN_SRC_STR[cfg->time_in_setting.source]);
        CPRINTF("Time Input Signal:\t%s\n", TIME_IN_SIGNAL_STR[cfg->time_in_setting.signal]);
        CPRINTF("\n");

        cli_header("Time Output Configuration", 1);
        CPRINTF("Time Output Type:\t%s\n", TIME_OUT_TYPE_STR[cfg->time_out_setting.timecode]);
        CPRINTF("Time Output Mode:\t%s\n", TIME_OUTPUT_MODE_STR[cfg->time_out_setting.mode]);
        CPRINTF("DC 1PPS Mode:\t%s\n", (cfg->time_out_setting.irig_dc_1pps_mode ? "On" : "Off"));
        CPRINTF("\n");
        cli_header("IEEE-1588 Configuration", 1);
        CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[cfg->time_out_setting.ieee_1588_type]);
        CPRINTF("\n");

        CPRINTF("TIME INPUT CALIBRATION:\n");
        for(i = 0; i < TIME_IN_CAL_TABLE_ENTRIES; i++)
        {
            CPRINTF("%s\n", TIME_IN_SRC_STR[i]);
            CPRINTF("\tCal:               %4X\n", cfg->time_in_calibration[i].cal);//, time_in_cal.small_cal);
            CPRINTF("\tOffset Multiplier: %4X\n", cfg->time_in_calibration[i].offset_multiplier);//, time_in_cal.small_cal);
        }
        CPRINTF("TIME OUTPUT CALIBRATION:\n");
        for(i = 0; i < TIME_OUT_CAL_TABLE_ENTRIES; i++)
        {
            CPRINTF("%s\n", TIME_OUT_TYPE_STR[i]);
            CPRINTF("\tLarge Cal: %X\n", cfg->time_out_calibration[i].coarse_cal);
            CPRINTF("\tMedium Cal: %X\n", cfg->time_out_calibration[i].med_cal);
            CPRINTF("\tFine Cal: %X\n", cfg->time_out_calibration[i].fine_cal);
        }

        cli_header("Discrete Input Configuration", 1);
        for(i = 0; i < DISCRETE_IN_MAX; i++)
        {
            CPRINTF("%d:\t", (i+1) );
            PrintDiscreteInput(cfg->di_config.di_setting[i]);
            CPRINTF("\n");
        }
        CPRINTF("\n");
        cli_header("Discrete Output Configuration", 1);
        for(i = 0; i < DISCRETE_OUT_MAX; i++)
        {
            CPRINTF("%d:\t", (i+1) );
            PrintDiscreteOutput(cfg->do_config.do_setting[i]);
            CPRINTF("\n");
        }
        CPRINTF("\n");
    }
    else
    {
        CPRINTF("CONFIG STRUCTURE INVALID!\n");
    }
}

static void FPGA_dot_info_cmd(cli_req_t * req)
{
    uint model, sn;

    get_board_revision( FLASH_CONF_UNIT_MODEL_TAG , &model);
    get_board_revision( FLASH_CONF_UNIT_SERIAL_NUM_TAG , &sn);

    CPRINTF("MANUFACTURER:  %s\n","Telspan Data" );
    CPRINTF("ADDRESS:       %s\n","7002 South Revere Parkway, Suite 60, Centennial, CO 80112"      );
    CPRINTF("CAGE CODE:     %s\n",":48U78");

    CPRINTF("MODEL:         %s\n", flash_conf_model[model].model);
    CPRINTF("DISCRIPTION:   %s\n","integrated Network Switch");
    if(sn == 0 )
        CPRINTF("UNIT SN:       %s\n", "Unit Serial Number has not been entered");
    else
        CPRINTF("UNIT SN:       %d\n", sn);
    CPRINTF("WEBSITE:       %s\n","www.telspandata.com");
    CPRINTF("EMAIL:         %s\n","support@telspandata.com");
}

static void FPGA_dot_debug_regdump_cmd(cli_req_t * req)
{
    int i;
    vns_register_info reginfo;
    u32 data;
    cli_header("FPGA REGISTER DUMP:", 1);
    for(i = 0; i < 0x20; i++)
    {
        reginfo.mask = 0xFFFFFFFF;
        reginfo.regnum = i;
        reginfo.shift = 0;
        if(_GetRegister(reginfo, &data))
        {
            CPRINTF("0x%02X:\t FAILED TO READ\n", i );
        }
        else
        {
            CPRINTF("0x%02X:\t 0x%08X\n", i, data);
        }
    }
}

#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)

static void FPGA_dot_gps_cmd(cli_req_t * req)
{
    vns_gps_dc_bias_t gps_config;
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_time_input time_in_cfg;

    if(Get_TimeInConfig(&time_in_cfg))
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        CPRINTF("\n");
    }
    else
    {
        if(fpga_req->set)
        {
            gps_config = fpga_req->gps_bias_setting;

            if(time_in_cfg.source == TIME_INPUT_SRC_GPS0 ||
                    time_in_cfg.source == TIME_INPUT_SRC_GPS3 ||
                    time_in_cfg.source == TIME_INPUT_SRC_GPS5 )
            {
                switch(gps_config)
                {
                    case DC_BIAS_0:
                        time_in_cfg.source = TIME_INPUT_SRC_GPS0;
                        break;
                    case DC_BIAS_3_3:
                        time_in_cfg.source = TIME_INPUT_SRC_GPS3;
                        break;
                    case DC_BIAS_5:
                        time_in_cfg.source = TIME_INPUT_SRC_GPS5;
                        break;
                    default:
                        PrintError(VNS_ERROR_COMMAND_FAILED);
                        break;
                }
                if(Set_TimeInConfig(time_in_cfg.source))
                {
                    PrintError(VNS_ERROR_COMMAND_FAILED);
                }
            }
            CPRINTF("\n");
        }
        else
        {
            if(!GetGPSAntennaBias(&gps_config))
            {
                cli_header("GPS Configuration", 1);
                CPRINTF("GPS DC Bias:\t%s\n", GPS_DC_BIAS_STR[gps_config]);
                CPRINTF("\n");
            }
            else
            {
                PrintError(VNS_ERROR_COMMAND_FAILED);
            }
        }
    }
}

#endif // defined (BOARD_VNS_12_REF)

static void FPGA_dot_health_cmd(cli_req_t * req)
{
    vns_fpga_req_t *fpga_req = req->module_req;

    uint32_t temp;
    if(fpga_req->feature < 0)
    {//show all
        GetSystemHealthReg(&temp);
        CPRINTF("%d %08X SYSTEM\n", FPGA_SUBSYSTEM_NONE, temp);
#if !defined(BOARD_IGU_REF)
        GetDiscreteInHealthReg(&temp);
        CPRINTF("%d %08X DISCRETE IN\n", FPGA_SUBSYSTEM_DISCRETE_INPUTS, temp);

        GetDiscreteOutHealthReg(&temp);
        CPRINTF("%d %08X DISCRETE OUT\n", FPGA_SUBSYSTEM_DISCRETE_OUTPUTS, temp);
#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
        GetGPSHealthReg(&temp);
        CPRINTF("%d %08X GPS\n", FPGA_SUBSYSTEM_GPS, temp);
#endif //defined(BOARD_VNS_12_REF)
#endif // !defined(BOARD_IGU_REF)
        GetTimeInHealth(&temp);
        CPRINTF("%d %08X TIME IN\n", FPGA_SUBSYSTEM_TIME_INPUT, temp);
#if !defined(BOARD_IGU_REF)
        GetTimeOutHealthReg(&temp);
        CPRINTF("%d %08X TIME OUT\n", FPGA_SUBSYSTEM_TIME_OUTPUT, temp);
#endif // !defined(BOARD_IGU_REF)
    }
    else
    {//show feature
        switch(fpga_req->feature)
        {
            case FPGA_SUBSYSTEM_NONE:
                GetSystemHealthReg(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X SYSTEM BIT Failure\n", FPGA_SUBSYSTEM_NONE, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X SYSTEM Setup Failure\n", FPGA_SUBSYSTEM_NONE, HEALTH_SETUP_FAIL);
                if(temp & SYS_HEALTH_OPERATION_FAIL)
                    CPRINTF("%d %08X SYSTEM Operation Failure\n", FPGA_SUBSYSTEM_NONE, SYS_HEALTH_OPERATION_FAIL);
                if(temp & SYS_HEALTH_BUSY)
                    CPRINTF("%d %08X SYSTEM Busy\n", FPGA_SUBSYSTEM_NONE, SYS_HEALTH_BUSY);
                if(temp & SYS_HEALTH_POWER_BOARD_OVER_TEMP)
                    CPRINTF("%d %08X SYSTEM Power Board Over-temp\n", FPGA_SUBSYSTEM_NONE, SYS_HEALTH_POWER_BOARD_OVER_TEMP);
                if(temp & SYS_HEALTH_FPGA_BOARD_PSC_FAIL)
                    CPRINTF("%d %08X SYSTEM Power System Controller Failure\n", FPGA_SUBSYSTEM_NONE, SYS_HEALTH_FPGA_BOARD_PSC_FAIL);
                if(temp & SYS_HEALTH_GPS_FAULT)
                    CPRINTF("%d %08X SYSTEM GPS Fault\n", FPGA_SUBSYSTEM_NONE, SYS_HEALTH_GPS_FAULT);
                break;
#if !defined(BOARD_IGU_REF)
            case FPGA_SUBSYSTEM_DISCRETE_INPUTS:
                GetDiscreteInHealthReg(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X DISCRETE IN BIT Failure\n", FPGA_SUBSYSTEM_DISCRETE_INPUTS, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X DISCRETE IN Setup Failure\n", FPGA_SUBSYSTEM_DISCRETE_INPUTS, HEALTH_SETUP_FAIL);
                break;
            case FPGA_SUBSYSTEM_DISCRETE_OUTPUTS:
                GetDiscreteOutHealthReg(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X DISCRETE OUT BIT Failure\n", FPGA_SUBSYSTEM_DISCRETE_OUTPUTS, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X DISCRETE OUT Setup Failure\n", FPGA_SUBSYSTEM_DISCRETE_OUTPUTS, HEALTH_SETUP_FAIL);
                break;
#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
            case FPGA_SUBSYSTEM_GPS:
                GetGPSHealthReg(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X GPS BIT Failure\n", FPGA_SUBSYSTEM_GPS, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X GPS Setup Failure\n", FPGA_SUBSYSTEM_GPS, HEALTH_SETUP_FAIL);
                if(temp & GPS_HEALTH_NO_ANTENNA_SIGNAL)
                    CPRINTF("%d %08X GPS No Signal\n", FPGA_SUBSYSTEM_GPS, GPS_HEALTH_NO_ANTENNA_SIGNAL);
                if(temp & GPS_HEALTH_BAD_ANTENNA_SIGNAL)
                    CPRINTF("%d %08X GPS Bad Signal\n", FPGA_SUBSYSTEM_GPS, GPS_HEALTH_BAD_ANTENNA_SIGNAL);
                if(temp & GPS_HEALTH_TIME_SYNC_FAIL)
                    CPRINTF("%d %08X GPS Time Sync Failure\n", FPGA_SUBSYSTEM_GPS, GPS_HEALTH_TIME_SYNC_FAIL);
                if(temp & GPS_HEALTH_NO_SATELLITES)
                    CPRINTF("%d %08X GPS No Satellites\n", FPGA_SUBSYSTEM_GPS, GPS_HEALTH_NO_SATELLITES);
                break;
#endif // defined(BOARD_VNS_12_REF)
#endif // !defined(BOARD_IGU_REF)
            case FPGA_SUBSYSTEM_TIME_INPUT:
                GetTimeInHealth(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X TIME IN BIT Failure\n", FPGA_SUBSYSTEM_TIME_INPUT, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X TIME IN Setup Failure\n", FPGA_SUBSYSTEM_TIME_INPUT, HEALTH_SETUP_FAIL);
                if(temp & TI_HEALTH_NO_SIGNAL)
                    CPRINTF("%d %08X TIME IN No Signal\n", FPGA_SUBSYSTEM_TIME_INPUT, TI_HEALTH_NO_SIGNAL);
                if(temp & TI_HEALTH_BAD_SIGNAL)
                    CPRINTF("%d %08X TIME IN Bad Signal\n", FPGA_SUBSYSTEM_TIME_INPUT, TI_HEALTH_BAD_SIGNAL);
                if(temp & TI_HEALTH_SYNC_FAIL)
                    CPRINTF("%d %08X TIME IN Sync Failure\n", FPGA_SUBSYSTEM_TIME_INPUT, TI_HEALTH_SYNC_FAIL);
                break;
#if !defined(BOARD_IGU_REF)
            case FPGA_SUBSYSTEM_TIME_OUTPUT:
                GetTimeOutHealthReg(&temp);
                if(temp & HEALTH_BIT_FAIL)
                    CPRINTF("%d %08X TIME OUT BIT Failure\n", FPGA_SUBSYSTEM_TIME_OUTPUT, HEALTH_BIT_FAIL);
                if(temp & HEALTH_SETUP_FAIL)
                    CPRINTF("%d %08X TIME OUT Setup Failure\n", FPGA_SUBSYSTEM_TIME_OUTPUT, HEALTH_SETUP_FAIL);
                if(temp & TO_HEALTH_NO_SOURCE)
                    CPRINTF("%d %08X TIME OUT No Source\n", FPGA_SUBSYSTEM_TIME_OUTPUT, TO_HEALTH_NO_SOURCE);
                if(temp & TO_HEALTH_TCG_FREEWHEEL)
                    CPRINTF("%d %08X TIME OUT Freewheel\n", FPGA_SUBSYSTEM_TIME_OUTPUT, TO_HEALTH_TCG_FREEWHEEL);
                break;
#endif //#if !defined(BOARD_IGU_REF)
        }

    }
}

static void FPGA_dot_igu_help_cmd(cli_req_t * req)
{
    CPRINTF(".1588 [master|slave|disable]\n");

    CPRINTF(".BIT\n");
    CPRINTF(".CONFIG\n");

    CPRINTF(".HEALTH [feature]\n");
    CPRINTF(".HELP\n");
    CPRINTF(".INFO\n");
    CPRINTF(".RESET\n");
    CPRINTF(".SAVE\n");
    CPRINTF(".STATUS\n");
    CPRINTF(".TIME\n");

    CPRINTF(".TIMIN [none|1588]\n");

    CPRINTF(".VERSION\n");

}
static void FPGA_dot_ies_6_help_cmd(cli_req_t * req)
{
#if HIDE_IRIG_INPUTS
    CPRINTF(".1588 [slave|disable]\n");
#else
    CPRINTF(".1588 [master|slave|disable]\n");
#endif

    CPRINTF(".BIT\n");
    CPRINTF(".CONFIG\n");

    CPRINTF(".DISCRETE [in|out] [<disc_num>] [<disc_setting>]\n");
    CPRINTF(".DC1PPS [on|off]\n");

    CPRINTF(".HEALTH [feature]\n");
    CPRINTF(".HELP\n");
    CPRINTF(".INFO\n");
    CPRINTF(".RESET\n");
    CPRINTF(".SAVE\n");
    CPRINTF(".STATUS\n");
    CPRINTF(".TIME\n");

#if HIDE_IRIG_INPUTS
    CPRINTF(".TIMIN [none|1588] \n");
#else
    CPRINTF(".TIMIN [none|a|b|g|1588] \n");
#endif

    CPRINTF(".TIMOUT [none|a|b|g]\n");
    CPRINTF(".TCAL [in|out] [cal1] [cal2] [cal3]\n");
    CPRINTF(".VERSION\n");

}
static void FPGA_dot_ies_8_help_cmd(cli_req_t * req)
{
    CPRINTF(".BIT\n");
    CPRINTF(".HELP\n");
    CPRINTF(".INFO\n");
    CPRINTF(".RESET\n");
    CPRINTF(".VERSION\n");
}
static void FPGA_dot_ies_12_help_cmd(cli_req_t * req)
{
    /* tsd_ch7_dec_debug_print(ENET_CH10_CH7_DECODER_2P0_0_BASE); */
    /* tsd_hdlc_dec_debug_print(ENET_HDLC_DECODER_2P0_0_BASE); */
    /* tsd_ch7_enc_debug_print(ENET_CH10_CH7_ENCODER_2P0_0_BASE); */
    /* tsd_hdlc_enc_debug_print(ENET_HDLC_ENCODER_2P0_0_BASE); */

    uint32_t base = ENET_CH10_CH7_ENCODER_2P0_0_BASE;

    double fpgaVersion, driverVersion;
    CPRINTF("CH7 Encoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_enc_get_version(base, &fpgaVersion, &driverVersion))
    {
        CPRINTF(".1588 [master|slave|disable]\n");
        CPRINTF(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        CPRINTF(" - Version:  FAILED TO READ\n");
    }

    CPRINTF(" - REGISTERS:\n");
    CPRINTF("    => %02X: VERSION REG            = 0x%08X\n", TSD_CH7_ENC_VERSION_REG           , TSD_CH7_ENC_IORD_VERSION(base));
    CPRINTF("    => %02X: IP ID REG              = 0x%08X\n", TSD_CH7_ENC_IP_ID_REG             , TSD_CH7_ENC_IORD_IP_ID(base));
    CPRINTF("    => %02X: STATUS REG             = 0x%08X\n", TSD_CH7_ENC_STATUS_REG            , TSD_CH7_ENC_IORD_STATUS(base));
    CPRINTF("    => %02X: CONTROL REG            = 0x%08X\n", TSD_CH7_ENC_CTRL_REG              , TSD_CH7_ENC_IORD_CONTROL(base));
    CPRINTF("    => %02X: PORT SELECT REG        = 0x%08X\n", TSD_CH7_ENC_PORT_SELECT_REG       , TSD_CH7_ENC_IORD_PORT_SELECT(base));
    CPRINTF("    => %02X: BITRATE REG            = 0x%08X\n", TSD_CH7_ENC_PORT_BIT_RATE_REG     , TSD_CH7_ENC_IORD_BITRATE(base));
    CPRINTF("    => %02X: SYNC PATTERN REG       = 0x%08X\n", TSD_CH7_ENC_SYNC_PATTERN_REG      , TSD_CH7_ENC_IORD_SYNC_PATTERN(base));
    CPRINTF("    => %02X: SYNC MASK REG          = 0x%08X\n", TSD_CH7_ENC_SYNC_MASK_REG         , TSD_CH7_ENC_IORD_SYNC_MASK(base));
    CPRINTF("    => %02X: SYNC LENGTH REG        = 0x%08X\n", TSD_CH7_ENC_SYNC_LENGTH_REG       , TSD_CH7_ENC_IORD_SYNC_LENGTH(base));
    CPRINTF("    => %02X: ETH FRAME CNT REG      = 0x%08X\n", TSD_CH7_ENC_ETH_RX_CNT_REG        , TSD_CH7_ENC_IORD_ETH_RX_CNT(base));
    CPRINTF("    => %02X: CH10 PACKET CNT REG    = 0x%08X\n", TSD_CH7_ENC_CH10_RX_CNT_REG       , TSD_CH7_ENC_IORD_CH10_RX_CNT(base));
    CPRINTF("    => %02X: ETH OVERFLOW CNT REG   = 0x%08X\n", TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG  , TSD_CH7_ENC_IORD_ETH_OVERFLOW_CNT(base));
    CPRINTF("    => %02X: ETH UNDERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG , TSD_CH7_ENC_IORD_ETH_UNDERFLOW_CNT(base));
    CPRINTF("    => %02X: CH10 OVERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG , TSD_CH7_ENC_IORD_CH10_OVERFLOW_CNT(base));
    CPRINTF("    => %02X: CH10 UNDERFLOW CNT REG = 0x%08X\n", TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG, TSD_CH7_ENC_IORD_CH10_UNDERFLOW_CNT(base));
    CPRINTF("    => %02X: PACKET ERROR CNT REG   = 0x%08X\n", TSD_CH7_ENC_CH10_PKT_ERR_CNT_REG  , TSD_CH7_ENC_IORD_PKT_ERR_CNT(base));

    CPRINTF("");
    base = ENET_CH10_CH7_DECODER_2P0_0_BASE;
    CPRINTF("CH7 Decoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_dec_get_version(base, &fpgaVersion, &driverVersion))
    {
        CPRINTF(" - FPGA Version:  %f\n", fpgaVersion);
        CPRINTF(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        CPRINTF(" - Version:  FAILED TO READ\n");
    }

    CPRINTF(" - REGISTERS:\n");
    CPRINTF("    => %02X: VERSION REG         = 0x%08X\n", TSD_CH7_DEC_VERSION_REG            , TSD_CH7_DEC_IORD_VERSION(base));
    CPRINTF("    => %02X: IP ID REG           = 0x%08X\n", TSD_CH7_DEC_IP_ID_REG              , TSD_CH7_DEC_IORD_IP_ID(base));
    CPRINTF("    => %02X: STATUS REG          = 0x%08X\n", TSD_CH7_DEC_STATUS_REG             , TSD_CH7_DEC_IORD_STATUS(base));
    CPRINTF("    => %02X: CONTROL REG         = 0x%08X\n", TSD_CH7_DEC_CTRL_REG               , TSD_CH7_DEC_IORD_CONTROL(base));
    CPRINTF("    => %02X: PORT SELECT REG     = 0x%08X\n", TSD_CH7_DEC_PORT_SELECT_REG        , TSD_CH7_DEC_IORD_PORT_SELECT(base));
    CPRINTF("    => %02X: SYNC PATTERN REG    = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_REG       , TSD_CH7_DEC_IORD_SYNC_PATTERN(base));
    CPRINTF("    => %02X: SYNC MASK REG       = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_MASK_REG  , TSD_CH7_DEC_IORD_SYNC_MASK(base));
    CPRINTF("    => %02X: SYNC LENGTH REG     = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_LEN_REG   , TSD_CH7_DEC_IORD_SYNC_LENGTH(base));
    CPRINTF("    => %02X: TX ETH PKT CNT REG  = 0x%08X\n", TSD_CH7_DEC_TX_ETH_PACKET_CNT_REG  , TSD_CH7_DEC_IORD_TX_ETH_PACKET_CNT(base));
    CPRINTF("    => %02X: TX CH10 PKT CNT REG = 0x%08X\n", TSD_CH7_DEC_TX_CH10_PACKET_CNT_REG , TSD_CH7_DEC_IORD_TX_CH10_PACKET_CNT(base));
    CPRINTF("    => %02X: DEC FIFO0 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO0_OVERFLOW_CNT(base));
    CPRINTF("    => %02X: DEC FIFO0 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO0_UNDERFLOW_CNT(base));
    CPRINTF("    => %02X: DEC FIFO1 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO1_OVERFLOW_CNT(base));
    CPRINTF("    => %02X: DEC FIFO1 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO1_UNDERFLOW_CNT(base));

    HdlcCh7DecoderInfo decoderinfo;
    HdlcCh7EncoderInfo encoderinfo;

    CPRINTF(" - ENCODE/DECODE:\n");
    encoderinfo.ch7EncoderBaseAddr  = ENET_CH10_CH7_ENCODER_2P0_0_BASE;
    encoderinfo.hdlcEncoderBaseAddr = ENET_HDLC_ENCODER_2P0_0_BASE;
    decoderinfo.ch7DecoderBaseAddr  = ENET_CH10_CH7_DECODER_2P0_0_BASE;
    decoderinfo.hdlcDecoderBaseAddr = ENET_HDLC_DECODER_2P0_0_BASE;
    CPRINTF("Encoder Underflow Count:        0x%X\n", getHdlcCh7EncoderUnderflowCount(&encoderinfo));
    CPRINTF("Encoder Overflow  Count:        0x%X\n", getHdlcCh7EncoderOverflowCount(&encoderinfo));
    CPRINTF("Encoder TSE Frame Count:        0x%X\n", getHdlcCh7EncoderEthFrameCount(&encoderinfo));
    CPRINTF("Encoder CH10 Frame Count:       0x%X\n", getHdlcCh7EncoderCH10FrameCount(&encoderinfo));
    CPRINTF("Decoder TSE Frame Count:        0x%X\n", getHdlcCh7DecoderEthFrameCount(&decoderinfo));
    CPRINTF("Decoder CH10 Frame Count:       0x%X\n", getCh7DecoderCH10FrameCount(&decoderinfo));
    CPRINTF("Decoder Underflow Count:        0x%X\n", getHdlcCh7DecoderUnderflowCount(&decoderinfo));
    CPRINTF("Decoder Overflow Count:         0x%X\n", getHdlcCh7DecoderOverflowCount(&decoderinfo));
    CPRINTF("Decoder Bit-Stuffing Err Count: 0x%X\n", getHdlcDecoderBitStuffingErrorCount(&decoderinfo));
    CPRINTF("Decoder Status:                 0x%LX\n", getHdlcCh7DecoderStatus(&decoderinfo));
    if(decoderinfo.decoderMode == TSD_HDLC_CH7_DECMODE_CH7) {
        CPRINTF("%s\n", getCh7DecoderStatusString(getHdlcCh7DecoderStatus(&decoderinfo)));
    } else {
        CPRINTF("%s\n", getHdlcDecoderStatusString(getHdlcCh7DecoderStatus(&decoderinfo)));
    }
#if false
    double version;
    base = TSE_SUBSYSTEM_ETH_TSE_0_BASE;
    CPRINTF("TSD TSE SXE Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_tse_sxe_driver_version(&version))
    {
        CPRINTF(" - Driver Version:  %f\n", version);
    }
    else
    {
        CPRINTF(" - Driver Version:  FAILED TO READ\n");
    }

    CPRINTF(" - REGISTERS:\n");
    CPRINTF("    => CMD_CONFIG        = 0x%08X\n", IORD_ALTERA_TSEMAC_CMD_CONFIG(base));
    CPRINTF("    => PCS_CONTROL       = 0x%08X\n", IORD_ALTERA_TSEPCS_CONTROL(base));
    CPRINTF("    => PCS_STATUS        = 0x%08X\n", IORD_ALTERA_TSEPCS_STATUS(base));
    CPRINTF("    => MAC_0             = 0x%08X\n", IORD_ALTERA_TSEMAC_MAC_0(base));
    CPRINTF("    => MAC_1             = 0x%08X\n", IORD_ALTERA_TSEMAC_MAC_1(base));
    CPRINTF("    => TX_CMD_STAT       = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_CMD_STAT(base));
    CPRINTF("    => RX_CMD_STAT       = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_CMD_STAT(base));
    CPRINTF("    => RX_ALMOST_EMPTY   = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_ALMOST_EMPTY(base));
    CPRINTF("    => RX_ALMOST_FULL    = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_ALMOST_FULL(base));
    CPRINTF("    => TX_ALMOST_EMPTY   = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_ALMOST_EMPTY(base));
    CPRINTF("    => TX_ALMOST_FULL    = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_ALMOST_FULL(base));
    CPRINTF("    => TX_SECTION_EMPTY  = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_SECTION_EMPTY(base));
    CPRINTF("    => TX_SECTION_FULL   = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_SECTION_FULL(base));
    CPRINTF("    => RX_SECTION_EMPTY  = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_SECTION_EMPTY(base));
    CPRINTF("    => RX_SECTION_FULL   = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_SECTION_FULL(base));
    CPRINTF("    => FRM_LENGTH        = 0x%08X\n", IORD_ALTERA_TSEMAC_FRM_LENGTH(base));
    CPRINTF("    => TX_IPG_LENGTH     = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_IPG_LENGTH(base));
    CPRINTF("    => TX_PAUSE_QUANT    = 0x%08X\n", IORD_ALTERA_TSEMAC_PAUSE_QUANT(base));
    CPRINTF("    => IF RX ERRORS      = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_ERRORS(base));
    CPRINTF("    => IF RX FCS ERRORS  = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAME_CHECK_SEQ_ERRS(base));
    CPRINTF("    => IF RX JABBBERS    = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_JABBERS(base));
    CPRINTF("    => IF RX FRAGMENTS   = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_FRAGMENTS(base));
    CPRINTF("    => IF TX ERRORS      = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_ERRORS(base));
    CPRINTF("    => IF TX DISCARDS    = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_DISCARDS(base));
    CPRINTF("    => IF TX UCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_UCAST_PKTS(base));
    CPRINTF("    => IF TX MCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_MULTICAST_PKTS(base));
    CPRINTF("    => IF TX BCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_BROADCAST_PKTS(base));
    CPRINTF("    => UNDERSIZE PKTS    = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_UNDERSIZE_PKTS(base));
    CPRINTF("    => OVERSIZE PKTS     = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_OVERSIZE_PKTS(base));
    CPRINTF("    => RX UNICAST PKTS   = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_UCAST_PKTS(base));
    CPRINTF("    => RX MULTICAST PKTS = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_MULTICAST_PKTS(base));
    CPRINTF("    => RX BROADCAST PKTS = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_BROADCAST_PKTS(base));
    CPRINTF("    => RX GOOD FRAMES    = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAMES_RX_OK(base));
    CPRINTF("    => TX GOOD FRAMES    = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAMES_TX_OK(base));
#endif


    //    CPRINTF(".1588 [master|slave|disable]\n");
    //
    //    CPRINTF(".BIT\n");
    //    CPRINTF(".CONFIG\n");
    //
    //    CPRINTF(".DISCRETE [in|out] [<disc_num>] [<disc_setting>]\n");
    //    CPRINTF(".DC1PPS [on|off]\n");
    //
    //    CPRINTF(".GPS [0|3|5]\n");
    //
    //    CPRINTF(".HEALTH [feature]\n");
    //    CPRINTF(".HELP\n");
    //    CPRINTF(".INFO\n");
    //    CPRINTF(".RESET\n");
    //    CPRINTF(".SAVE\n");
    //    CPRINTF(".STATUS\n");
    //    CPRINTF(".TIME\n");
    //
    //    CPRINTF(".TIMIN [none|a|b|g|gps|1588|1pps] [am|dc]\n");
    //
    //    CPRINTF(".TIMOUT [none|a|b|g]\n");
    //    CPRINTF(".TCAL [in|out] [cal1] [cal2] [cal3]\n");
    //    CPRINTF(".VERSION\n");
}
static void FPGA_dot_help_cmd(cli_req_t * req)
{

#if defined (BOARD_VNS_6_REF)
    FPGA_dot_ies_6_help_cmd(req);
#elif defined ( BOARD_VNS_8_REF )
    FPGA_dot_ies_8_help_cmd(req);
#elif defined ( BOARD_VNS_12_REF )
    FPGA_dot_ies_12_help_cmd(req);
#elif defined ( BOARD_VNS_16_REF )
    FPGA_dot_ies_12_help_cmd(req);
#elif defined ( BOARD_IGU_REF )
    FPGA_dot_igu_help_cmd(req);
#else
#error "Need to write help function!"

// #if defined (BOARD_VNS_6_REF)
// #if HIDE_IRIG_INPUTS
//     CPRINTF(".1588 [master|slave|disable]\n");
// #else
//     CPRINTF(".1588 [slave|disable]\n");
// #endif
// 
// #if HIDE_IRIG_INPUTS
// #if defined (BOARD_VNS_12_REF)
//     CPRINTF(".1588 [master|slave|disable]\n");
// #else
//     CPRINTF(".1588 [slave|disable]\n");
// #endif
// #else
//     CPRINTF(".1588 [master|slave|disable]\n");
// #endif
//     CPRINTF(".BIT\n");
//     CPRINTF(".CONFIG\n");
// #if !defined(BOARD_IGU_REF)
//     CPRINTF(".DISCRETE [in|out] [<disc_num>] [<disc_setting>]\n");
//     CPRINTF(".DC1PPS [on|off]\n");
// #endif // !defined(BOARD_IGU_REF)
// #if defined (BOARD_VNS_12_REF)
//     CPRINTF(".GPS [0|3|5]\n");
// #endif
//     CPRINTF(".HEALTH [feature]\n");
//     CPRINTF(".HELP\n");
//     CPRINTF(".INFO\n");
//     CPRINTF(".RESET\n");
//     CPRINTF(".SAVE\n");
//     CPRINTF(".STATUS\n");
//     CPRINTF(".TIME\n");
// #if HIDE_IRIG_INPUTS
// #if defined (BOARD_VNS_12_REF)
//     CPRINTF(".TIMIN [none|gps|1588]\n");
// #else
//     CPRINTF(".TIMIN [none|1588]\n");
// #endif
// #else
// #if defined (BOARD_VNS_12_REF)
//     CPRINTF(".TIMIN [none|a|b|g|gps|1588|1pps] [am|dc]\n");
// #else
//     CPRINTF(".TIMIN [none|a|b|g|1588] \n");
// #endif
// #endif
// 
// #if !defined(BOARD_IGU_REF)
//     CPRINTF(".TIMOUT [none|a|b|g]\n");
//     CPRINTF(".TCAL [in|out] [cal1] [cal2] [cal3]\n");
// #endif // !defined(BOARD_IGU_REF)
//     CPRINTF(".VERSION\n");

#endif
}

static void FPGA_dot_reset_cmd(cli_req_t * req)
{
    int rc;
    vtss_restart_t restart = VTSS_RESTART_COOL;

    CPRINTF("System will reboot in a few seconds\n");
    rc = control_system_reset(TRUE, VTSS_ISID_LOCAL, restart);

    if (rc) {
        CPRINTF("Restart failed! System is updating by another process.\n");
    }
}

static void FPGA_dot_save_cmd(cli_req_t * req)
{

    if(!save_vns_config())
    {
        CPRINTF("Configuration was successfully saved...\n");
    }
    else
    {
        CPRINTF("ERROR: Failed saving configuration...\n");
    }

}

static void FPGA_dot_time_cmd(cli_req_t * req)
{
    int year, yday, hour,minute, second;
    vns_time_input_src_t time_in_src;
    vns_fpga_req_t *fpga_req = req->module_req;
    vtss_timestamp_t time;
    if(0 == Get_TimeInSource(&time_in_src))
    {
        if(fpga_req->set)
        {
            if( fpga_req->time.command == IES_TIME_PROCESS_COMMAND_SET)
            {
                time.seconds = FpgaTime_2_time_t(
                        fpga_req->time.year,
                        fpga_req->time.yday,
                        fpga_req->time.hour,
                        fpga_req->time.minute,
                        fpga_req->time.second);
                /* vtss_local_clock_time_set(&time, 0); */
                cyg_libc_time_settime( time.seconds );
                set_1pps_time(
                        fpga_req->time.year,
                        fpga_req->time.yday,
                        fpga_req->time.hour,
                        fpga_req->time.minute,
                        fpga_req->time.second
                        );
        
        /* int years, int ydays, int hours, int minutes, int seconds, int milliseconds); */

            }
            else if( fpga_req->time.command == IES_TIME_PROCESS_COMMAND_ADD)
            {
                modify_input_time( IES_TIME_PROCESS_COMMAND_ADD, fpga_req->time.second);
            }
            else if( fpga_req->time.command == IES_TIME_PROCESS_COMMAND_SUBTRACT)
            {
                modify_input_time( IES_TIME_PROCESS_COMMAND_SUBTRACT, fpga_req->time.second);
            }
            else
                CPRINTF("Unknown input entered...");

        }
        else 
        {
            switch(time_in_src)
            {
                case TIME_INPUT_SRC_1588:
                    if(0 == Get_PTP_SetTime(&year, &yday, &hour, &minute, &second))
                    {
                        CPRINTF("Time In:  %03d-%02d:%02d:%02d\n", yday, hour, minute, second);
                    }
                    break;
                case TIME_INPUT_SRC_1PPS:
                default:
                    if(0 == Get_TimeIn_Time(&year, &yday, &hour, &minute, &second))
                    {
                        CPRINTF("Time In:  %03d-%02d:%02d:%02d\n", yday, hour, minute, second);
                    }
                    break;
            }
#if false /* !defined(BOARD_IGU_REF) */
            if(0 == Get_TimeOut_Time(&year, &yday, &hour, &minute, &second))
            {
                CPRINTF("Time Out: %03d-%02d:%02d:%02d\n", yday, hour, minute, second);
            }
#endif /* BOARD_IGU_REF */
        }
    }
    else
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
    }
    //CPRINTF("Command not yet implemented...\n");
}

static void FPGA_dot_time_in_cmd(cli_req_t * req)
{
    /* vns_time_input time_config; */
    vns_time_input_src_t time_config;
    vns_fpga_req_t *fpga_req = req->module_req;
    u8 fw_version_major, fw_version_minor;
    vns_gps_dc_bias_t gps_config;
    
    if(fpga_req->set)
    {
        /* vns_time_input_src_t source; */
        set_time_input_config(fpga_req->time_in_setting);
        save_vns_config();
        /* set_time_input_config( vns_time_input_src_t source ); */
    }
    else {

        vns_time_input_src_t  current_src;
        /* int Get_CurrentTimeInSource(vns_time_input_src_t * src) */
        if(Get_CurrentTimeInSource(&current_src))
        {
            PrintError( VNS_ERROR_COMMAND_FAILED);
        }
        if(Get_TimeInConfig(&time_config))
        {
            PrintError( VNS_ERROR_COMMAND_FAILED);
        }

        CPRINTF("Current Time Source:\t%s\n", TIME_IN_SRC_STR[current_src]);
        CPRINTF("Configured Time Source:\t%s\n", TIME_IN_SRC_STR[time_config]);
        /* CPRINTF("Time Input Signal:\t%s\n", TIME_IN_SIGNAL_STR[time_config.signal]); */
        CPRINTF("\n");
#if false
        if(!GetGPSAntennaBias(&gps_config))
        {
            cli_header("GPS Configuration", 1);
            CPRINTF("GPS DC Bias:\t%s\n", GPS_DC_BIAS_STR[gps_config]);
            CPRINTF("\n");
        }
        else
        {
            CPRINTF("GetGPSAntennaBias Failed\n");
            PrintError( VNS_ERROR_COMMAND_FAILED);
        }
#endif

    }
}

static void FPGA_dot_time_in_cmd_org(cli_req_t * req)
{
    vns_time_input time_config;
    vns_fpga_req_t *fpga_req = req->module_req;
    u8 fw_version_major, fw_version_minor;


    if(Get_TimeInConfig(&time_config))
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
    }
    else
    {
        if(fpga_req->set)
        {
            // if( fpga_req->time_in_setting == TIME_INPUT_SRC_1PPS ||
            //         fpga_req->time_in_setting == TIME_INPUT_SRC_IRIG_A ||
            //         fpga_req->time_in_setting == TIME_INPUT_SRC_IRIG_B ||
            //         fpga_req->time_in_setting == TIME_INPUT_SRC_IRIG_G )
            // {
            //     /* so not all switches support these inputs so, we need to block the user from using them */
            //     if(!is_1pps_in_able()) 
            //     {
            //         CPRINTF("Current hardware set is not able to receive a 1PPS input\n");
            //         CPRINTF("Please contact vendor for further details\n");
            //         return;
            //     }
            //     if(!is_irig_in_able()) 
            //     {
            //         CPRINTF("Current hardware set is not able to receive an IRIG input\n");
            //         CPRINTF("Please contact vendor for further details\n");
            //         return;
            //     }
            //     if( is_mac_ies12() )
            //     {
            //         if(Get_FPGAFirmwareVersionMajor(&fw_version_major) ||
            //                 Get_FPGAFirmwareVersionMinor(&fw_version_minor))
            //         { CPRINTF("Error occured getting firmware version\n"); return; }
            //         if( fw_version_major < 3 || fw_version_minor < 1)
            //         {
            //             CPRINTF("Current firmware version %d.%d does not support IRIG & 1PPS input\n", fw_version_major, fw_version_minor);
            //             CPRINTF("Must have firmware version 3.1 or greater.\n");
            //             return;
            //         }
            //     }

            //     time_config.signal = TIME_INPUT_TYPE_DC;
            // }
            if((fpga_req->time_in_setting == TIME_INPUT_SRC_GPS0) &&
                    (time_config.source == TIME_INPUT_SRC_GPS0 ||
                     time_config.source == TIME_INPUT_SRC_GPS3 ||
                     time_config.source == TIME_INPUT_SRC_GPS5) )
            {
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else
            {
                time_config.source = fpga_req->time_in_setting;
                if(time_config.source == TIME_INPUT_SRC_1588)
                {
                    time_config.signal = TIME_INPUT_TYPE_DC;
                }
                else if(time_config.source == TIME_INPUT_SRC_1PPS)
                {
                    time_config.signal = TIME_INPUT_TYPE_DC;
                    /* create_ptp_clock(true); */
                    /* if(Set_IEEE_1588_Mode( IEEE_1588_SLAVE ) ) */
                    /* { */
                    /*     CPRINTF("COMMAND FAILED\n"); */
                    /*     PrintError(VNS_ERROR_COMMAND_FAILED); */
                    /* } */
                }
                else
                {
                    if(fpga_req->time_in_signal != TIME_INPUT_TYPE_RESERVED) {
                        time_config.signal = fpga_req->time_in_signal;
                    }
                    else {
                        //time_config.signal = TIME_INPUT_TYPE_AM; /* No AM signals yet */
                        time_config.signal = TIME_INPUT_TYPE_DC;
                    }
                }
                if(Set_TimeInConfig(time_config.source))
                {
                    PrintError(VNS_ERROR_COMMAND_FAILED);
                }
                else
                {
                    save_vns_config();
                }
            }

            CPRINTF("\n");
        }
        else
        {
            cli_header("Time Input Configuration", 1);
            CPRINTF("Time Input Source:\t%s\n", TIME_IN_SRC_STR[time_config.source]);
            CPRINTF("Time Input Signal:\t%s\n", TIME_IN_SIGNAL_STR[time_config.signal]);
            CPRINTF("\n");
        }
    }
}
static void FPGA_dot_time_out_cmd(cli_req_t * req)
{
    vns_time_output time_config;
    vns_fpga_req_t *fpga_req = req->module_req;

    if(fpga_req->set)
    {
        //time_config.timecode = fpga_req->time_out_setting;
        if(Set_TimeOutTimeCode(fpga_req->time_out_setting))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
        }
        else
        {
            save_vns_config();
        }
        CPRINTF("\n");
    }
    else
    {
        if(!Get_TimeOutConfig(&time_config))
        {
            cli_header("Time Output Configuration", 1);
            CPRINTF("Time Output Type:\t%s\n", TIME_OUT_TYPE_STR[time_config.timecode]);
            /* CPRINTF("Time Output Mode:\t%s\n", TIME_OUTPUT_MODE_STR[time_config.mode]); */
            //CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[time_config.ieee_1588_type]);
            CPRINTF("\n");
        }
        else
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
        }
    }
}

static void FPGA_dot_dc_1pps_cmd(cli_req_t * req)
{
    BOOL dc1ppsmode;
    vns_fpga_req_t *fpga_req = req->module_req;

    if(fpga_req->set)
    {
        //time_config.timecode = fpga_req->time_out_setting;
        if(Set_Ttl1ppsMode(fpga_req->onoff))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
        }
        else
        {
            save_vns_config();
        }
        CPRINTF("\n");
    }
    else
    {
        if(!Get_Ttl1ppsMode(&dc1ppsmode))
        {
            cli_header("IRIG-DC Output Mode:", 1);
            CPRINTF("%s\n", (dc1ppsmode ? "1PPS":"IRIG-DC"));
            CPRINTF("\n");
        }
        else
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
        }
    }
}

static void FPGA_dot_1588_cmd(cli_req_t * req)
{
    vns_1588_type ptp_mode;
    vns_fpga_req_t *fpga_req = req->module_req;

    if(Get_IEEE_1588_Mode(&ptp_mode))
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        CPRINTF("\n");
    }
    else
    {
        if(fpga_req->set)
        {
            /* if (fpga_req->ieee_1588_mode == IEEE_1588_SLAVE) */
                    /* Set_TimeInSource(TIME_INPUT_SRC_1588); */

            if(Set_IEEE_1588_Mode_Mod_clk(fpga_req->ieee_1588_mode, fpga_req->modify_clock))
            {
                CPRINTF("COMMAND FAILED\n");
                PrintError(VNS_ERROR_COMMAND_FAILED);
            }
            else
            {
                save_vns_config();
            }
        }
        else
        {
            cli_header("IEEE-1588 Configuration", 1);
            CPRINTF("IEEE-1588 Mode:\t%s\n", IEEE_1588_TYPE_STR[ptp_mode]);
        }
    }
}

static void FPGA_dot_tcal_cmd(cli_req_t * req)
{
    vns_fpga_req_t *fpga_req = req->module_req;

    vns_time_in_cal time_in_cal;
    vns_time_out_cal time_out_cal;
    if(Get_TimeInCalibration(&time_in_cal))
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
    if(Get_TimeOutCalibration(&time_out_cal))
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
    if(fpga_req->set)
    {//set new values
        if(fpga_req->direction == DIRECTION_IN)
        {//set input cal
            if(fpga_req->cal_1 != -1)
            {
                time_in_cal.cal = fpga_req->cal_1;//((fpga_req->cal_1 & 0xFF00) >> 8);
                //				time_in_cal.small_cal = (fpga_req->cal_1 & 0x00FF);
                //				time_in_cal.rtc_cal = 0; //unused
            }
            if(Set_TimeInCalibration(time_in_cal))
            {
                PrintError(VNS_ERROR_COMMAND_FAILED);
                return;
            }
            else
            {
                save_vns_config();
            }
        }
        else if(fpga_req->direction == DIRECTION_OUT)
        {//set output cal
            if(fpga_req->cal_1 != -1)
            {
                time_out_cal.fine_cal = fpga_req->cal_1;
            }
            if(fpga_req->cal_2 != -1)
            {
                time_out_cal.med_cal = fpga_req->cal_2;
            }
            if(fpga_req->cal_3 != -1)
            {
                time_out_cal.coarse_cal = fpga_req->cal_3;
            }
            if(Set_TimeOutCalibration(time_out_cal))
            {
                PrintError(VNS_ERROR_COMMAND_FAILED);
                return;
            }
            else
            {
                save_vns_config();
            }
        }
        else
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
    }
    else
    {//display current values
        if((fpga_req->direction == DIRECTION_IN) ||
                (fpga_req->direction == DIRECTION_NONE))
        {//print input cal
            CPRINTF("TIME INPUT CALIBRATION:\n");
            CPRINTF("\tCal: %4X\n", time_in_cal.cal);//, time_in_cal.small_cal);
        }
        if((fpga_req->direction == DIRECTION_OUT) ||
                (fpga_req->direction == DIRECTION_NONE))
        {//print output cal
            CPRINTF("TIME OUTPUT CALIBRATION:\n");
            CPRINTF("\tLarge Cal: %X\n", time_out_cal.coarse_cal);
            CPRINTF("\tMedium Cal: %X\n", time_out_cal.med_cal);
            CPRINTF("\tFine Cal: %X\n", time_out_cal.fine_cal);
        }
    }

}

static void FPGA_dot_version_cmd(cli_req_t * req)
{

    CPRINTF("PCKG: v%d.%02d,", VNS_VERSION_COMMON_IP, VNS_VERSION_PCKG_ID);
    CPRINTF("%s v%d.%02d\n", MODEL_NAME, VNS_VERSION_MAJOR, VNS_VERSION_MINOR);
/* #if defined( BOARD_VNS_6_REF ) || defined(BOARD_VNS_8_REF) || defined(BOARD_VNS_12_REF) */
    uint32_t fw_version_major = 0, fw_version_minor = 0;
    if(Get_FPGAFirmwareVersionMinor(&fw_version_minor) || Get_FPGAFirmwareVersionMajor(&fw_version_major)) {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
    CPRINTF("%s v%08X.%06X\n","FPGA", fw_version_major, fw_version_minor);
/* #endif */
}
static void FPGA_dot_disout_cmd(cli_req_t * req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    u32 do_state = 0;
    if(fpga_req->set)
    {
        if(Set_discretes_out(fpga_req->disc_out_val))
        {
            PrintError(VNS_ERROR_COMMAND_FAILED);
            return;
        }
    }

    if(Get_DiscreteOutputState(&do_state))//int Get_DiscreteOutputState(u32 * do_state)
    {
        PrintError(VNS_ERROR_COMMAND_FAILED);
        return;
    }
    CPRINTF("Discrete Output Status: Ox%03X\n",((~do_state)& DISCRETE_OUT_MASK));
}
static void FPGA_set_product_id_cmd(cli_req_t * req)
{
    conf_board_t conf;
    uint serial_number = 0;


    if (conf_mgmt_board_get(&conf) < 0)
        return;
    conf.mac_address[3] = VNS_MAC_PRODUCT_ID;
    if (conf_mgmt_board_set(&conf) != VTSS_OK)
        CPRINTF("Board set operation failed\n");

    serial_number = conf.mac_address[5] & 0xFF;
    serial_number |= conf.mac_address[4] << 8 & 0xFF00;
    CPRINTF("Board Serial number: %s SW %d\n", vtss_board_name(), serial_number);
    CPRINTF("MAC product ID 0x%02X\n",  conf.mac_address[3]);

}
static void FPGA_dot_snrev5_cmd(cli_req_t * req)
{
    conf_board_t conf;
    uint serial_number = 0;


    if (conf_mgmt_board_get(&conf) < 0)
        return;

    if (req->set) {
        serial_number = req->int_values[0];

        if ( MAC_ADDRESS_MAX_VALUE <= serial_number || 0 > serial_number) {
            CPRINTF("Error: value must be less than %d or greater than zero.  %d was entered\n", MAC_ADDRESS_MAX_VALUE, serial_number);
            return;
        }
        if(conf.mac_address[5] != 0 || conf.mac_address[4] != 0) {
            CPRINTF("Error: (Serial number <=> MAC address) link has already been made\n");
            return;
        }


        conf.mac_address[5] = serial_number & 0xFF;
        conf.mac_address[4] = serial_number >> 8 & 0xFF;
        conf.mac_address[3] = VNS_MAC_PRODUCT_ID;

        if (conf_mgmt_board_set(&conf) != VTSS_OK)
            CPRINTF("Board set operation failed\n");
    } else {
        serial_number = conf.mac_address[5] & 0xFF;
        serial_number |= conf.mac_address[4] << 8 & 0xFF00;
        CPRINTF("Board Serial number: %s SW %d\n", vtss_board_name(), serial_number);
        CPRINTF("MAC product ID 0x%02X\n",  conf.mac_address[3]);

    }
}
static void FPGA_dot_sn_cmd(cli_req_t * req)
{
    conf_board_t conf;
    uint serial_number = 0;


    if (conf_mgmt_board_get(&conf) < 0)
        return;

    if (req->set) {
        serial_number = req->int_values[0];

        if ( MAC_ADDRESS_MAX_VALUE <= serial_number || 0 > serial_number) {
            CPRINTF("Error: value must be less than %d or greater than zero.  %d was entered\n", MAC_ADDRESS_MAX_VALUE, serial_number);
            return;
        }
        if(conf.mac_address[5] != 0 || conf.mac_address[4] != 0) {
            CPRINTF("Error: (Serial number <=> MAC address) link has already been made\n");
            return;
        }


        conf.mac_address[5] = serial_number & 0xFF;
        conf.mac_address[4] = serial_number >> 8 & 0xFF;
        conf.mac_address[3] = VNS_MAC_PRODUCT_ID;

        if (conf_mgmt_board_set(&conf) != VTSS_OK)
            CPRINTF("Board set operation failed\n");
    } else {
        serial_number = conf.mac_address[5] & 0xFF;
        serial_number |= conf.mac_address[4] << 8 & 0xFF00;
        CPRINTF("Board Serial number: %s SW %d\n", vtss_board_name(), serial_number);
        CPRINTF("MAC product ID 0x%02X\n",  conf.mac_address[3]);

    }
}
/****************************************************************************/
/*  Parameter functions                                                     */
/****************************************************************************/

static int32_t cli_fpga_i2c_data_word_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;
    int len = strlen(cmd);
    BOOL isValid = true;
    int i = 0;

    while(i < len)
    {
        if(!isxdigit(cmd[i]))
        {
            isValid = false;
        }
        i++;
    }
    if(isValid)
    {
        if(1 == sscanf(cmd, "%X", &value))
        {
            fpga_req->data_word = value;
            return 0;
        }
    }
    return 1;
}


static int32_t cli_fpga_i2c_offset_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    //int32_t        error;
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;
    BOOL isValid = true;
    int i = 0;
    int len = strlen(cmd);

    while(i < len)
    {
        if(!isxdigit(cmd[i]))
        {
            isValid = false;
        }
        i++;
    }
    if(isValid)
    {
        if(1 == sscanf(cmd, "%X", &value))
        {
            fpga_req->reg_offset = value;
            return 0;
        }
    }
    //error = cli_parse_ulong(cmd, &value, 0, 255);
    //fpga_req->reg_addr = value;
    //return error;
    return 1;
}

static int32_t cli_fpga_i2c_register_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    //int32_t        error;
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;
    BOOL isValid = true;
    int i = 0;
    int len = strlen(cmd);

    while(i < len)
    {
        if(!isxdigit(cmd[i]))
        {
            isValid = false;
        }
        i++;
    }
    if(isValid)
    {
        if(1 == sscanf(cmd, "%X", &value))
        {
            fpga_req->reg_addr = value;
            return 0;
        }
    }
    //error = cli_parse_ulong(cmd, &value, 0, 255);
    //fpga_req->reg_addr = value;
    //return error;
    return 1;
}

static int32_t cli_fpga_direction_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;
    error = VTSS_OK;

    fpga_req->direction = DIRECTION_NONE;
    if(0 == strncmp(cmd, "in", 2))
        fpga_req->direction = DIRECTION_IN;
    else if(0 == strncmp(cmd, "out", 3))
        fpga_req->direction = DIRECTION_OUT;
    else
    {
        error = VTSS_INVALID_PARAMETER;
    }

    return VTSS_OK;
}

static int32_t cli_fpga_onoff_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;
    error = VTSS_OK;

    fpga_req->direction = DIRECTION_NONE;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "on", 2))
        fpga_req->onoff = TRUE;
    else if(0 == strncmp(cmd, "off", 3))
        fpga_req->onoff = FALSE;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }

    return VTSS_OK;
}

static int32_t cli_fpga_035_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "0", 1))
        fpga_req->gps_bias_setting = DC_BIAS_0;
    else if(0 == strncmp(cmd, "3", 1))
        fpga_req->gps_bias_setting = DC_BIAS_3_3;
    else if(0 == strncmp(cmd, "5", 1))
        fpga_req->gps_bias_setting = DC_BIAS_5;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}

static int32_t cli_fpga_feature_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;

    error = cli_parse_ulong(cmd, &value, 0, 255);
    fpga_req->feature = value;
    return error;
}

static int32_t cli_fpga_disc_num_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;

    error = cli_parse_ulong(cmd, &value, 0, 255);
    fpga_req->disc_num = value;
    return error;
}

static int32_t cli_fpga_disc_setting_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    ulong          value = 0;
    vns_fpga_req_t *fpga_req = req->module_req;

    error = cli_parse_ulong(cmd, &value, 0, 255);
    if(fpga_req->direction == DIRECTION_IN)
    {
        if(value == 0)
        {
            fpga_req->disc_in_setting = value;
            fpga_req->set = TRUE;
        }
        else
        {
            fpga_req->set = FALSE;
            error = VTSS_INVALID_PARAMETER;
        }
    }
    else if(fpga_req->direction == DIRECTION_OUT)
    {
        if((value >= 0 && value <= DO_SETTING_DISABLED) )
        {
            fpga_req->disc_out_setting = value;
            fpga_req->set = TRUE;
        }
        else
        {
            fpga_req->set = FALSE;
            error = VTSS_INVALID_PARAMETER;
        }
    }
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }

    return error;
}

static int32_t cli_fpga_time_in_cfg_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    //"none|gps|1588"
    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "none", 4))
        fpga_req->time_in_setting = TIME_INPUT_SRC_DISABLED;
    else if(0 == strncmp(cmd, "gps_0v", 6))
        fpga_req->time_in_setting = TIME_INPUT_SRC_GPS0;
    else if(0 == strncmp(cmd, "gps_3v", 6))
        fpga_req->time_in_setting = TIME_INPUT_SRC_GPS3;
    else if(0 == strncmp(cmd, "gps_5v", 6))
        fpga_req->time_in_setting = TIME_INPUT_SRC_GPS5;
    else if(0 == strncmp(cmd, "1pps", 4))
        fpga_req->time_in_setting = TIME_INPUT_SRC_1PPS;
    else if(0 == strncmp(cmd, "b_dc", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_B_DC;
    else if(0 == strncmp(cmd, "a_dc", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_A_DC;
    else if(0 == strncmp(cmd, "g_dc", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_G_DC;
    else if(0 == strncmp(cmd, "b_am", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_B_AM;
    else if(0 == strncmp(cmd, "a_am", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_A_AM;
    else if(0 == strncmp(cmd, "g_am", 3))
        fpga_req->time_in_setting = TIME_INPUT_SRC_IRIG_G_AM;
    else if(0 == strncmp(cmd, "1588", 4))
        fpga_req->time_in_setting = TIME_INPUT_SRC_1588;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}
static int32_t cli_fpga_am_dc_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    //"none|gps|1588"
    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "am", 2))
        fpga_req->time_in_signal = TIME_INPUT_TYPE_AM;
    else if(0 == strncmp(cmd, "dc", 1))
        fpga_req->time_in_signal = TIME_INPUT_TYPE_DC;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}
static int32_t cli_fpga_time_out_cfg_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    //"none|a|b|g"
    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "none", 4))
        fpga_req->time_out_setting = TIME_OUTPUT_TYPE_DISABLED;
    else if(0 == strncmp(cmd, "a", 1))
        fpga_req->time_out_setting = TIME_OUTPUT_TYPE_IRIG_A;
    else if(0 == strncmp(cmd, "b", 1))
        fpga_req->time_out_setting = TIME_OUTPUT_TYPE_IRIG_B;
    else if(0 == strncmp(cmd, "g", 1))
        fpga_req->time_out_setting = TIME_OUTPUT_TYPE_IRIG_G;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}

static int32_t cli_fpga_disc_val_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    ulong value = 0;
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    //"none|a|b|g"
    error = VTSS_OK;

    int len = strlen(cmd);
    BOOL isValid = true;

    int i = 0;

    fpga_req->set = TRUE;

    while(i < len)
    {
        if(!isxdigit(cmd[i]))
        {
            isValid = false;
        }
        i++;
    }
    if(isValid)
    {
        if(1 == sscanf(cmd, "%X", &value))
        {
            if(fpga_req->disc_out_val == -1)
            {
                fpga_req->disc_out_val = value;
            }
        }
        else
        {
            fpga_req->set = FALSE;
            error = VTSS_INVALID_PARAMETER;
        }
    }
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}
static int32_t cli_fpga_cal_num_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    ulong value = 0;
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    //"none|a|b|g"
    error = VTSS_OK;

    int len = strlen(cmd);
    BOOL isValid = true;

    int i = 0;

    fpga_req->set = TRUE;

    while(i < len)
    {
        if(!isxdigit(cmd[i]))
        {
            isValid = false;
        }
        i++;
    }
    if(isValid)
    {
        if(1 == sscanf(cmd, "%X", &value))
        {
            if(fpga_req->cal_1 == -1)
            {
                fpga_req->cal_1 = value;
            }
            else if(fpga_req->cal_2 == -1)
            {
                fpga_req->cal_2 = value;
            }
            else
            {
                fpga_req->cal_3 = value;
            }
        }
        else
        {
            fpga_req->set = FALSE;
            error = VTSS_INVALID_PARAMETER;
        }
    }
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}

static int32_t cli_fpga_modify_clk_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;
    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "mod", 3))
        fpga_req->modify_clock = TRUE;
    else
        fpga_req->modify_clock = FALSE;

    return VTSS_OK;
}
static int32_t cli_fpga_1588_mode_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    int32_t        error;
    vns_fpga_req_t *fpga_req = req->module_req;

    error = VTSS_OK;
    fpga_req->set = TRUE;
    if(0 == strncmp(cmd, "disable", 7))
        fpga_req->ieee_1588_mode = IEEE_1588_DISABLED;
    else if(0 == strncmp(cmd, "master", 6))
        fpga_req->ieee_1588_mode = IEEE_1588_MASTER;
    else if(0 == strncmp(cmd, "slave", 5))
        fpga_req->ieee_1588_mode = IEEE_1588_SLAVE;
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return error;
}

static int32_t cli_param_epe_license_section_a (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    /* CPRINTF("cmd: %s\n", cmd); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        fpga_req->license_info.reserved1 = value;
        /* req->value=value; */
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_param_epe_license_section_b (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    /* CPRINTF("cmd: %s\n", cmd); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        fpga_req->license_info.reserved2 = value;
        /* req->value=value; */
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_param_epe_license_section_c (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    /* CPRINTF("cmd: %s\n", cmd); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        fpga_req->license_info.epe_signature = value;
        /* req->value=value; */
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_param_epe_license_section_d (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    /* CPRINTF("cmd: %s\n", cmd); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        fpga_req->license_info.ies_signature = value;
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_param_epe_license_section_e (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    /* CPRINTF("cmd: %s\n", cmd); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        fpga_req->license_info.key = value;
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}


static int32_t cli_param_date_time (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int n = 1;
    int error = 0; 
    vns_time_t time;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;

    if(5 == sscanf(cmd, "%04d-%03d-%02d:%02d:%02d", &time.year, &time.yday, &time.hour, &time.minute, &time.second))
    {
        time.command = IES_TIME_PROCESS_COMMAND_SET;
        fpga_req->time = time;
        /* CPRINTF("cli_param_date_time time set %d\n", n); */
    }
    else if(0 == strncmp(cmd, "+", 1))
    {
        fpga_req->time.command = IES_TIME_PROCESS_COMMAND_ADD;
        if(1 == sscanf(cmd, "+%d", &n)) {  }
        /* CPRINTF("cli_param_date_time add %d\n", n); */
    }
    else if(0 == strncmp(cmd, "-", 1))
    {
        fpga_req->time.command = IES_TIME_PROCESS_COMMAND_SUBTRACT;
        if(1 == sscanf(cmd, "-%d", &n)) {  }
        /* CPRINTF("cli_param_date_time subtract %d\n", n); */
    }
    else
    {
        error = 1;
        /* CPRINTF("date_time scanf did not match"); */
    }
    if(!error)
    {
        fpga_req->set = TRUE;
        if(fpga_req->time.command != IES_TIME_PROCESS_COMMAND_SET)
            fpga_req->time.second = n;
    }

    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_param_hex_ulong (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    ulong   value = 0,
            min   = 0,
            max = 0xFFFFFFFF;
    req->parm_parsed = 1;
    /* int cli_parse_hex_ulong(char *cmd, ulong *req, ulong min, ulong max); */
    int error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if(!error) {
        req->value=value;
    }
    //   CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static int32_t cli_fpga_epe_multi_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    req->parm_parsed = 1;
    int error = cli_parse_integer(cmd, req, NULL);
    vns_fpga_req_t *fpga_req = req->module_req;
    if(error)
        return error;
    int val = req->int_values[req->int_value_cnt -1];
    /* int port_num; */
    /* vns_time_t time; */
    if(req->int_value_cnt == 1 ) {
        if( val < VNS_PORT_COUNT +1 && val > 0 ){
            fpga_req->port_num = val;
            /* CPRINTF("Setting Port Num: %d \n", val); */
        }
        else {
            /* CPRINTF("Port out of rang: %d \n", val); */
            return -1;
        }
        CPRINTF("cmd 1\n");
    }
    else if(req->int_value_cnt == 2 ) {
        fpga_req->time_seconds = val;
        /* CPRINTF("cmd 2\n"); */
        /* CPRINTF("Setting Delay: %d \n", val); */
        req->set = TRUE;
    }
    /* CPRINTF("cli_fpga_short_parse %d, %d\n",fpga_req->port_num,fpga_req->time_seconds  ); */

    /* CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n" */
            /* , cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt); */
    return error;
}

static int32_t cli_fpga_int_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    req->parm_parsed = 1;
    int error = cli_parse_integer(cmd, req, NULL);

    CPRINTF("cli_fpga_short_parse: cmd %s, value entered: %d, number of values %d\n", cmd, req->int_values[req->int_value_cnt -1], req->int_value_cnt);
    return error;
}

static void cli_cmd_epe_port(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf = fpga_req->epe_config;
    /* epe_conf.insert_fcs = TRUE; */
    if (req->set)
    {  
        cli_cmd_epe_internal( req );
    } else {
        cli_cmd_epe_internal_get(req);
    }
}

static void cli_cmd_epe_multi_port(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf = fpga_req->epe_config;
    /* epe_conf.insert_fcs = TRUE; */
    if (req->set)
    {  
        cli_cmd_epe_internal( req );
    } else {
        cli_cmd_epe_internal_get(req);
    }
}

static void cli_cmd_epe_encoder_status(cli_req_t *req)
{
    /* uint32_t temp; */
    // **** For Getting time
    /* int warning_bits = 0; // not sure */
    /* int error_bits = 0; // count number of health bits set (all systems, I think) */
    /* int i; */
    /* uint32_t fw_version_major = 0, fw_version_minor = 0; */

    uint32_t base = ENET_CH10_CH7_DECODER_2P0_0_BASE;

    double fpgaVersion, driverVersion;


    base = ENET_CH10_CH7_ENCODER_2P0_0_BASE;

    CPRINTF( 
        "* CH7 Encoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_enc_get_version(base, &fpgaVersion, &driverVersion))
    {
        CPRINTF( 
            " - FPGA Version:  %f\n", fpgaVersion);
        CPRINTF( 
            " - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        CPRINTF( 
            " - Version:  FAILED TO READ\n");
    }

    CPRINTF( 
        " - REGISTERS:\n");
    CPRINTF( 
        "    => %02X: VERSION REG            = 0x%08X\n", TSD_CH7_ENC_VERSION_REG           , TSD_CH7_ENC_IORD_VERSION(base));
    CPRINTF( 
        "    => %02X: IP ID REG              = 0x%08X\n", TSD_CH7_ENC_IP_ID_REG             , TSD_CH7_ENC_IORD_IP_ID(base));
    CPRINTF( 
        "    => %02X: STATUS REG             = 0x%08X\n", TSD_CH7_ENC_STATUS_REG            , TSD_CH7_ENC_IORD_STATUS(base));
    CPRINTF( 
        "    => %02X: CONTROL REG            = 0x%08X\n", TSD_CH7_ENC_CTRL_REG              , TSD_CH7_ENC_IORD_CONTROL(base));
    CPRINTF( 
        "    => %02X: PORT SELECT REG        = 0x%08X\n", TSD_CH7_ENC_PORT_SELECT_REG       , TSD_CH7_ENC_IORD_PORT_SELECT(base));
    CPRINTF( 
        "    => %02X: BITRATE REG            = 0x%08X\n", TSD_CH7_ENC_PORT_BIT_RATE_REG     , TSD_CH7_ENC_IORD_BITRATE(base));
    CPRINTF( 
        "    => %02X: SYNC PATTERN REG       = 0x%08X\n", TSD_CH7_ENC_SYNC_PATTERN_REG      , TSD_CH7_ENC_IORD_SYNC_PATTERN(base));
    CPRINTF( 
        "    => %02X: SYNC MASK REG          = 0x%08X\n", TSD_CH7_ENC_SYNC_MASK_REG         , TSD_CH7_ENC_IORD_SYNC_MASK(base));
    CPRINTF( 
        "    => %02X: SYNC LENGTH REG        = 0x%08X\n", TSD_CH7_ENC_SYNC_LENGTH_REG       , TSD_CH7_ENC_IORD_SYNC_LENGTH(base));
    CPRINTF( 
        "    => %02X: ETH FRAME CNT REG      = 0x%08X\n", TSD_CH7_ENC_ETH_RX_CNT_REG        , TSD_CH7_ENC_IORD_ETH_RX_CNT(base));
    CPRINTF( 
        "    => %02X: CH10 PACKET CNT REG    = 0x%08X\n", TSD_CH7_ENC_CH10_RX_CNT_REG       , TSD_CH7_ENC_IORD_CH10_RX_CNT(base));
    CPRINTF( 
        "    => %02X: ETH OVERFLOW CNT REG   = 0x%08X\n", TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG  , TSD_CH7_ENC_IORD_ETH_OVERFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: ETH UNDERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG , TSD_CH7_ENC_IORD_ETH_UNDERFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: CH10 OVERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG , TSD_CH7_ENC_IORD_CH10_OVERFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: CH10 UNDERFLOW CNT REG = 0x%08X\n", TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG, TSD_CH7_ENC_IORD_CH10_UNDERFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: PACKET ERROR CNT REG   = 0x%08X\n", TSD_CH7_ENC_CH10_PKT_ERR_CNT_REG  , TSD_CH7_ENC_IORD_PKT_ERR_CNT(base));

#if false

    base = ENET_HDLC_ENCODER_2P0_0_BASE;

    CPRINTF( 
        "* HDLC Encoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_hdlc_enc_get_version(base, &fpgaVersion, &driverVersion))
    {
    CPRINTF( 
            " - FPGA Version:  %f\n", fpgaVersion);
    CPRINTF( 
            " - Driver Version:  %f\n", driverVersion);
    }
    else
    {
    CPRINTF( 
            " - Version:  FAILED TO READ\n");
    }

    CPRINTF( 
        " - REGISTERS:\n");
    CPRINTF( 
        "    => %02X: VERSION REG        = 0x%08X\n", TSD_HDLC_ENC_VERSION_REG        , TSD_HDLC_ENC_IORD_VERSION(base));
    CPRINTF( 
        "    => %02X: IP ID REG          = 0x%08X\n", TSD_HDLC_ENC_IP_ID_REG          , TSD_HDLC_ENC_IORD_IP_ID(base));
    CPRINTF( 
        "    => %02X: STATUS REG         = 0x%08X\n", TSD_HDLC_ENC_STATUS_REG         , TSD_HDLC_ENC_IORD_STATUS(base));
    CPRINTF( 
        "    => %02X: CONTROL REG        = 0x%08X\n", TSD_HDLC_ENC_CTRL_REG           , TSD_HDLC_ENC_IORD_CONTROL(base));
    CPRINTF( 
        "    => %02X: BITRATE REG        = 0x%08X\n", TSD_HDLC_ENC_BITRATE_REG        , TSD_HDLC_ENC_IORD_BITRATE(base));
    CPRINTF( 
        "    => %02X: PACKET COUNT REG   = 0x%08X\n", TSD_HDLC_ENC_PACKET_CNT_REG     , TSD_HDLC_ENC_IORD_PACKET_CNT(base));
    CPRINTF( 
        "    => %02X: FIFO0 OFLOW CNT    = 0x%08X\n", TSD_HDLC_ENC_FIFO0_OFLOW_CNT_REG, TSD_HDLC_ENC_IORD_FIFO0_OFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: FIFO0 UFLOW CNT    = 0x%08X\n", TSD_HDLC_ENC_FIFO0_UFLOW_CNT_REG, TSD_HDLC_ENC_IORD_FIFO0_UFLOW_CNT(base));

#endif
    CPRINTF( "\n");
}

static void cli_cmd_epe_decoder_status(cli_req_t *req)
{

    uint32_t base = ENET_CH10_CH7_DECODER_2P0_0_BASE;

    double fpgaVersion, driverVersion;
    CPRINTF("* CH7 Decoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_dec_get_version(base, &fpgaVersion, &driverVersion))
    {
        CPRINTF( 
                " - FPGA Version:  %f\n", fpgaVersion);
        CPRINTF( 
                " - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        CPRINTF( " - Version:  FAILED TO READ\n");
    }

    CPRINTF( 
        " - REGISTERS:\n");
    CPRINTF( 
        "    => %02X: VERSION REG         = 0x%08X\n", TSD_CH7_DEC_VERSION_REG            , TSD_CH7_DEC_IORD_VERSION(base));
    CPRINTF( 
        "    => %02X: IP ID REG           = 0x%08X\n", TSD_CH7_DEC_IP_ID_REG              , TSD_CH7_DEC_IORD_IP_ID(base));
    CPRINTF( 
        "    => %02X: STATUS REG          = 0x%08X\n", TSD_CH7_DEC_STATUS_REG             , TSD_CH7_DEC_IORD_STATUS(base));
    CPRINTF( 
        "    => %02X: CONTROL REG         = 0x%08X\n", TSD_CH7_DEC_CTRL_REG               , TSD_CH7_DEC_IORD_CONTROL(base));
    CPRINTF( 
        "    => %02X: PORT SELECT REG     = 0x%08X\n", TSD_CH7_DEC_PORT_SELECT_REG        , TSD_CH7_DEC_IORD_PORT_SELECT(base));
    CPRINTF( 
            "    => %02X: SYNC PATTERN REG    = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_REG       , TSD_CH7_DEC_IORD_SYNC_PATTERN(base));
    CPRINTF( 
            "    => %02X: SYNC MASK REG       = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_MASK_REG  , TSD_CH7_DEC_IORD_SYNC_MASK(base));
    CPRINTF( 
            "    => %02X: SYNC LENGTH REG     = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_LEN_REG   , TSD_CH7_DEC_IORD_SYNC_LENGTH(base));
    CPRINTF( 
            "    => %02X: TX ETH PKT CNT REG  = 0x%08X\n", TSD_CH7_DEC_TX_ETH_PACKET_CNT_REG  , TSD_CH7_DEC_IORD_TX_ETH_PACKET_CNT(base));
    CPRINTF( 
            "    => %02X: TX CH10 PKT CNT REG = 0x%08X\n", TSD_CH7_DEC_TX_CH10_PACKET_CNT_REG , TSD_CH7_DEC_IORD_TX_CH10_PACKET_CNT(base));
    CPRINTF( 
            "    => %02X: DEC FIFO0 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO0_OVERFLOW_CNT(base));
    CPRINTF( 
            "    => %02X: DEC FIFO0 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO0_UNDERFLOW_CNT(base));
    CPRINTF( 
            "    => %02X: DEC FIFO1 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO1_OVERFLOW_CNT(base));
    CPRINTF( 
            "    => %02X: DEC FIFO1 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO1_UNDERFLOW_CNT(base));


#if false
    base = ENET_HDLC_DECODER_2P0_0_BASE;

    T_D("HDLC Decoder Core ");
    CPRINTF( 
            "* HDLC Decoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_hdlc_dec_get_version(base, &fpgaVersion, &driverVersion))
    {
        CPRINTF( 
            " - FPGA Version:  %f\n", fpgaVersion);
        CPRINTF( 
            " - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        CPRINTF( 
        " - Version:  FAILED TO READ\n");
    }

    CPRINTF( 
        " - REGISTERS:\n");
    CPRINTF( 
        "    => %02X: VERSION REG          = 0x%08X\n", TSD_HDLC_DEC_VERSION_REG             , TSD_HDLC_DEC_IORD_VERSION(base));
    CPRINTF( 
        "    => %02X: IP ID REG            = 0x%08X\n", TSD_HDLC_DEC_IP_ID_REG               , TSD_HDLC_DEC_IORD_IP_ID(base));
    CPRINTF( 
        "    => %02X: STATUS REG           = 0x%08X\n", TSD_HDLC_DEC_STATUS_REG              , TSD_HDLC_DEC_IORD_STATUS(base));
    CPRINTF( 
        "    => %02X: CONTROL REG          = 0x%08X\n", TSD_HDLC_DEC_CTRL_REG                , TSD_HDLC_DEC_IORD_CONTROL(base));
    CPRINTF( 
        "    => %02X: PACKET CNT REG       = 0x%08X\n", TSD_HDLC_DEC_PACKET_CNT_REG          , TSD_HDLC_DEC_IORD_PACKET_CNT(base));
    CPRINTF( 
        "    => %02X: FIFO0 OFLOW CNT      = 0x%08X\n", TSD_HDLC_DEC_FIFO0_OFLOW_CNT_REG     , TSD_HDLC_DEC_IORD_FIFO0_OFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: FIFO0 UFLOW CNT      = 0x%08X\n", TSD_HDLC_DEC_FIFO0_UFLOW_CNT_REG     , TSD_HDLC_DEC_IORD_FIFO0_UFLOW_CNT(base));
    CPRINTF( 
        "    => %02X: BIT STUFFING ERR CNT = 0x%08X\n", TSD_HDLC_DEC_STUFFING_ERR_CNT_REG     , TSD_HDLC_DEC_IORD_STUFFING_ERR_CNT(base));
    /* CPRINTF(  */

#endif
    CPRINTF( "\n");
}

static void cli_cmd_epe_debug(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf = fpga_req->epe_config;
    /* epe_conf.insert_fcs = TRUE; */
    cli_cmd_epe_internal( req );
}

static void cli_cmd_epe_multi(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    int port_num = fpga_req->port_num;
    int time_delay = fpga_req->time_seconds;
    vns_fpga_conf_t * cfg = get_shadow_config();
    int delay, i = 0;

    if (req->set) {
        /* cfg->epe_multi_time_delay[fpga_req->port_num-1] = fpga_req->time_seconds; */
        set_multi_time_delay( fpga_req->port_num, fpga_req->time_seconds);
        /* CPRINTF("set_multi_time_delay %d, %d\n",fpga_req->port_num,fpga_req->time_seconds  ); */
    }
    else {
        CPRINTF(" Multi Port Mode\n");
        CPRINTF("==================\n");
        for( i= 0; i < VNS_PORT_COUNT; i++ ) {

            get_multi_time_delay( i +1 , &delay);
            CPRINTF(" %d: %d\n", i + 1,delay );

        }


    }

    /* CPRINTF("CLI_CMD_EPE_MULTI %d, %d\n",fpga_req->port_num,fpga_req->time_seconds  ); */

}
static void FPGA_dot_led_test(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
#if false
    set_leds_debug(fpga_req->led_red_blue, 
            fpga_req->led_green, 
            fpga_req->led_flashing, 
            fpga_req->discrete_out);
#endif
    eLEDColor_t color;
    int port, shft = 0;
    eLED_t start_led;
    uint32_t val;
    int i = 0;
    eFrontPanelBoard_t front_panel = get_front_panel_board();
    CPRINTF("Running Init\n");
    init_led_subsystem();
    // INIT_LED_SUBSYSTEM(LED_CONTROLLER_BASE, ALT_CPU_FREQ, front_panel);


    tsd_shift_reg_led_clear_all(front_panel, TSD_SHIFT_REG_LED_ACTIVE_HIGH); /* clear all bits */
    // DISABLE ALL
    for( i = 0; i < IES_LED_UNUSED; i++)
    {
        CPRINTF("Testing LED %s\n",ELED_NAME_STRING[i]);

        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_RED, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_GREEN, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_BLUE, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_WHITE, true);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_OFF, false);
    }

}
 
void cli_cmd_epe_internal_get(cli_req_t *req)
{
    mirror_conf_t        conf;
    mirror_mgmt_conf_get(&conf);
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf = fpga_req->epe_config;

    if (!get_vns_fpga_epe_decode_conf(&epe_conf) ) {
        CPRINTF("\nCould not get VNS FPGA EPE config\n");
        return;
    }
    CPRINTF("\n\n========= Decoder Config =========\n");
    CPRINTF("    Mode:\t%s\n", VNS_EPE_MODE_STR[epe_conf.mode]);
    CPRINTF("    BitRate: %u \n", epe_conf.bit_rate );
    if(epe_conf.mode == VNS_EPE_CH7_15 || epe_conf.mode == VNS_EPE_FULLDPX_CH7_15 || epe_conf.mode == VNS_EPE_DECODE_CH7_15    )
        CPRINTF("    Word(s) in minor frame:\t%d\n", epe_conf.ch7_data_word_count);
    else
        CPRINTF("    Word(s) in minor frame:\tNA\n");
    CPRINTF("    Clock:\t%s\n", epe_conf.invert_clock ? "Inverted" : "Default");
    CPRINTF("    FCS:\t%s\n", epe_conf.insert_fcs ? "Inserted" : "Removed");



    if (!get_vns_fpga_epe_encode_conf(&epe_conf) ) {
        CPRINTF("\nCould not get VNS FPGA EPE config\n");
        return;
    }
    CPRINTF("\n========= Encoder Config ========= \n");
    CPRINTF("    Mode:\t%s\n", VNS_EPE_MODE_STR[epe_conf.mode]);
    CPRINTF("    BitRate: %u \n", epe_conf.bit_rate );
    if(epe_conf.mode == VNS_EPE_CH7_15 || epe_conf.mode == VNS_EPE_FULLDPX_CH7_15 || epe_conf.mode == VNS_EPE_DECODE_CH7_15    )
        CPRINTF("    Word(s) in minor frame:\t%d\n", epe_conf.ch7_data_word_count);
    else
        CPRINTF("    Word(s) in minor frame:\tNA\n");
    CPRINTF("    Clock:\t%s\n", epe_conf.invert_clock ? "Inverted" : "Default");
    CPRINTF("    FCS:\t%s\n", epe_conf.insert_fcs ? "Inserted" : "Removed");

    CPRINTF("\n========= Mirror Config ========= \n");
    if (iport2uport(conf.dst_port) == VNS_EPE_PORT ) {
        CPRINTF("    Warning: Mirror configuration is being used. FPGA port may block traffic if using decoder.\n");
    }
    else {
        CPRINTF("    Warning: If using encoder only mode, you may need to set Mirror destination port to the FPGA, port %d .\n", VNS_EPE_PORT);
        /* CPRINTF("EPE Mode is %s. Mirror configuration is being used.\n", VNS_EPE_MODE_STR[VNS_EPE_DISABLE]); */
    }
#if false
#endif
}

void cli_cmd_epe_internal(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf = fpga_req->epe_config;

#if false
    CPRINTF("\n");
    if (req->set)
        CPRINTF("SET\n");
    else
        CPRINTF("GET\n");

    if(fpga_req->decode)
        CPRINTF("decode\n");
    if(fpga_req->encode)
        CPRINTF("encode\n");
    if(fpga_req->disable)
        CPRINTF("disable\n");

    CPRINTF("bit_rate: %d\n",fpga_req->epe_config.bit_rate);
    CPRINTF("mode: %s\n",VNS_EPE_MODE_STR[ fpga_req->epe_config.mode ] );
    CPRINTF("ch7_data_word_count: %d\n", fpga_req->epe_config.ch7_data_word_count );
                
    if ( fpga_req->epe_config.invert_clock )
        CPRINTF("Invert Clock\n");
    else
        CPRINTF("Clock\n");

    if ( fpga_req->epe_config.insert_fcs )
        CPRINTF("Insert FCS\n");
    else
        CPRINTF("Remove FCS\n");

    CPRINTF("*************\n");
#endif
    /* set_vns_fpga_epe_conf(epe_conf); */
    /* save_vns_config(); */
    /* return; */
    if( fpga_req->encode ) {
        /* CPRINTF("Encoder\n"); */
        /* activate_epe_decoder( NULL ); */
        if(fpga_req->disable)
            fpga_req->epe_config.mode = VNS_EPE_DISABLE;
        activate_epe_encoder( &epe_conf );
    }
    else if( fpga_req->decode ) {
        /* CPRINTF("Decode\n"); */

        /* Need to convert to decoder values */
        if( fpga_req->epe_config.mode == VNS_EPE_CH7_15 )
            epe_conf.mode = VNS_EPE_DECODE_CH7_15;
        else if( fpga_req->epe_config.mode == VNS_EPE_HDLC )
            epe_conf.mode = VNS_EPE_DECODE_HDLC;
        else/* if(fpga_req->disable) Doing the same */
            fpga_req->epe_config.mode = VNS_EPE_DISABLE;
        activate_epe_decoder( &epe_conf );
    }
    else {
        return;
        /* epe_config_disable_all(); */
    }
#if false
#endif
    save_vns_config();

   /* int result = activate_epe(); */
}

#ifdef IES_EPE_FEATURE_ENABLE
static void cli_cmd_epe_port_org(cli_req_t *req)
{
    mirror_conf_t        conf;
    mirror_mgmt_conf_get(&conf);
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_epe_conf_blk_t epe_conf;
    if( !is_epe_able() ) {
        CPRINTF("EPE configuration may not be available for your hardware configuration.\n");
        CPRINTF("Please contact vendor.\n");
        return;
    }
    if(!is_epe_license_acitive()) {
        CPRINTF("Ethernet PCM Encode/Decode license is not valid.\n");
        CPRINTF("Please contact vendor.\n");
        return;
    }
    if (req->set)
    {  
        if (iport2uport(conf.dst_port) == VNS_EPE_PORT ||
                conf.dst_port == VTSS_PORT_NO_NONE) 
        {
            /* CPRINTF("User Bitrate: %d.\n", fpga_req->epe_config.bit_rate ); */
            /* CPRINTF("User mode: %d.\n", fpga_req->epe_config.mode ); */
            /* CPRINTF("User frame size: %d.\n", fpga_req->epe_config.ch7_data_word_count ); */
            /* if( fpga_req->encode ) */
            /*     CPRINTF("encode true.\n"); */
            /* else */
            /*     CPRINTF("encode false.\n"); */
            /*  */
            /* if( fpga_req->decode ) */
            /*     CPRINTF("decode true.\n"); */
            /* else */
            /*     CPRINTF("decode false.\n"); */

            epe_conf.bit_rate            = (( fpga_req->epe_config.bit_rate* 100 )/ VNS_EPE_BIT_RATE_MAX);

            epe_conf.mode                = fpga_req->epe_config.mode;
            epe_conf.ch7_data_word_count = fpga_req->epe_config.ch7_data_word_count;
            if( fpga_req->disable == TRUE)
            {
                epe_conf.mode      = VNS_EPE_DISABLE;
                /* conf.dst_port      = MIRROR_PORT_DEFAULT; // Disabling mirroring */
                /* conf.mirror_switch = MIRROR_SWITCH_DEFAULT; */
            }
            else
            {
                if( !fpga_req->decode && !fpga_req->encode )
                {
                    CPRINTF("Must set operation.\n");
                    CPRINTF("Please include the value, encode or decode.\n\n");
                    return;
                }
                if( epe_conf.mode == VNS_EPE_DISABLE )
                {
                    CPRINTF("Mode was not selected.\n");
                    CPRINTF("Please include the value, hdlc or ch7.\n\n");
                    return;
                }
                if( fpga_req->epe_config.bit_rate < VNS_EPE_BIT_RATE_MIN 
                        || fpga_req->epe_config.bit_rate > VNS_EPE_BIT_RATE_MAX )
                {
                    if( fpga_req->encode == TRUE )
                    {
                        CPRINTF("Bit rate is not valid.\n");
                        CPRINTF("Please include the value, %d-%d.\n\n", VNS_EPE_BIT_RATE_MIN, VNS_EPE_BIT_RATE_MAX);
                        return;
                    }
                }
                if( epe_conf.mode == VNS_EPE_CH7_15 || epe_conf.mode == VNS_EPE_DECODE_CH7_15 )
                {
                    if( epe_conf.ch7_data_word_count < VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MIN 
                            || epe_conf.ch7_data_word_count > VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MAX )
                    {
                        CPRINTF("CH7 frame word count was not valid or set.\n");
                        CPRINTF("Please include the value, 1-8.\n\n");
                        return;
                    }
                    /* decoder is not being set in parser */
                    if( fpga_req->decode == TRUE )
                        epe_conf.mode = VNS_EPE_DECODE_CH7_15;
                }
                else if( epe_conf.mode == VNS_EPE_HDLC || epe_conf.mode == VNS_EPE_DECODE_HDLC )
                {
                    /* decoder is not being set in parser */
                    if( fpga_req->decode == TRUE )
                        epe_conf.mode = VNS_EPE_DECODE_HDLC;
                }
                else
                {
                    CPRINTF("Invalid PCM Mode...\n");
                    return;
                }
            }
            
            if( fpga_req->decode == TRUE && !is_epe_decoder_able() )
            {
                CPRINTF("The decoder is not avalible with the current FPGA load.\n");
                CPRINTF("Please update FPGA to 3.7 or greater.\n\n");
                return;
            }

            /* CPRINTF("EPE: Setting BitRate to %u%% of a 20MHz clock, %u kbps\n", epe_conf.bit_rate, (epe_conf.bit_rate * VNS_EPE_BIT_RATE_MAX)/100); */
            /* CPRINTF("EPE: Setting BitRate to %u% , %u  min, %u  max\n", epe_conf.bit_rate, VNS_EPE_BIT_RATE_MIN, VNS_EPE_BIT_RATE_MAX); */
            /* CPRINTF("EPE: Setting Mode to %s\n", VNS_EPE_MODE_STR[epe_conf.mode]); */
            /* if(epe_conf.mode == VNS_EPE_CH7_15) */
            /*     CPRINTF("Word(s) in minor frame:\t%d\n", epe_conf.ch7_data_word_count); */
            /* else */
            /*     CPRINTF("Word(s) in minor frame:\tNA\n"); */
            /* set_vns_fpga_hdlc(bit_rate); */
            if ( !set_vns_fpga_epe_conf(epe_conf) ) {
                CPRINTF("\nCould not set VNS FPGA EPE config\n");
            }
            if( fpga_req->disable == TRUE || fpga_req->decode == TRUE )
            {
                conf.dst_port      = MIRROR_PORT_DEFAULT; // Disabling mirroring
                conf.mirror_switch = MIRROR_SWITCH_DEFAULT;
            }
            else
            {
                conf.dst_port = uport2iport( VNS_EPE_PORT );
            }
            if (mirror_mgmt_conf_set(&conf) != VTSS_OK) {
                CPRINTF("\nCould not set Mirror mgmt config\n");
            }
        }
        else {
            CPRINTF("Please disable mirror configuration to enable EPE mode.\n");
        }
    }
    else{ 

        if (!get_vns_fpga_epe_conf(&epe_conf) ) {
            CPRINTF("\nCould not get VNS FPGA EPE config\n");
        }
        if (iport2uport(conf.dst_port) == VNS_EPE_PORT || 
                conf.dst_port == MIRROR_PORT_DEFAULT) {
            CPRINTF("EPE: BitRate divider: %u%% of a 20MHz clock, %u kbps\n", epe_conf.bit_rate, (epe_conf.bit_rate * VNS_EPE_BIT_RATE_MAX)/100);
            CPRINTF("EPE Mode:\t%s\n", VNS_EPE_MODE_STR[epe_conf.mode]);
            /* CPRINTF("EPE BitRate %u%% of a 20MHz clock, %2.2f MHz\n", epe_conf.bit_rate, epe_conf.bit_rate * 20 * 0.01); */
            if(epe_conf.mode == VNS_EPE_CH7_15)
                CPRINTF("Word(s) in minor frame:\t%d\n", epe_conf.ch7_data_word_count);
            else
                CPRINTF("Word(s) in minor frame:\tNA\n");
        }
        else {
            CPRINTF("EPE Mode is %s. Mirror configuration is being used.\n", VNS_EPE_MODE_STR[VNS_EPE_DISABLE]);
        }
    }
}

static int32_t cli_epe_parse_keyword (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    int error = 0;


    if (0 == (error = cli_parse_word(cmd, "enable"))) {
        req->enable = 1;
    } else if (0 == (error = cli_parse_word(cmd, "disable"))) {
        fpga_req->disable = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "encode"))) {
        fpga_req->encode = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "decode"))) {
        fpga_req->decode = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "decoder_status"))) {
        req->enable = 1;
    } else if (0 == (error = cli_parse_word(cmd, "encoder_status"))) {
        req->enable = 1;
    } else if (0 == (error = cli_parse_word(cmd, "full_duplex"))) {
        fpga_req->encode = TRUE;
        fpga_req->decode = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "rx"))) {
        fpga_req->rx = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "tx"))) {
        fpga_req->tx = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "hdlc"))) {
        fpga_req->epe_config.mode = VNS_EPE_HDLC;
    } else if (0 == (error = cli_parse_word(cmd, "ch7"))) {
        fpga_req->epe_config.mode = VNS_EPE_CH7_15;
    } else if (0 == (error = cli_parse_word(cmd, "invert_clock"))) {
        fpga_req->epe_config.invert_clock = TRUE;
    } else if (0 == (error = cli_parse_word(cmd, "remove_fcs"))) {
        fpga_req->epe_config.insert_fcs = FALSE;
    }
    if(error == 0 )
        req->parm_parsed = 1;

    return (error);
}
#endif /* IES_EPE_FEATURE_ENABLE */
static int32_t cli_board_rev_parse_keyword (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    int error = 0;

    req->parm_parsed = 1;

    
    if (0 == (error = cli_parse_word(cmd, "power"))) {
        fpga_req->board = FLASH_CONF_POWER_REV_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "fpga"))) {
        fpga_req->board = FLASH_CONF_FPGA_REV_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "switch"))) {
        fpga_req->board = FLASH_CONF_SWITCH_REV_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "interconnect"))) {
        fpga_req->board = FLASH_CONF_INTERCONNECT_REV_BRD_TAG;
    }
    return (error);
}
static int32_t cli_board_id_parse_keyword (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    int error = 0;

    req->parm_parsed = 1;

    
    if (0 == (error = cli_parse_word(cmd, "power"))) {
        fpga_req->board = FLASH_CONF_POWER_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "fpga"))) {
        fpga_req->board = FLASH_CONF_FPGA_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "switch"))) {
        fpga_req->board = FLASH_CONF_SWITCH_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "interconnect"))) {
        fpga_req->board = FLASH_CONF_INTERCONNECT_BRD_TAG;
    } else if (0 == (error = cli_parse_word(cmd, "unit_sn"))) {
        fpga_req->board = FLASH_CONF_UNIT_SERIAL_NUM_TAG;
    /* } else if (0 == (error = cli_parse_word(cmd, "ies12"))) { */
    } else if (0 == (error = cli_parse_word( cmd ,"ies12" ))) {
        fpga_req->board = FLASH_CONF_UNIT_MODEL_TAG;
        fpga_req->ies_board_model = FLASH_CONF_BRD_MODEL_DRS_VNS_12;
    /* } else if (0 == (error = cli_parse_word(cmd, "ies6"))) { */
    } else if (0 == (error = cli_parse_word( cmd ,"ies6" ))) {
        fpga_req->board = FLASH_CONF_UNIT_MODEL_TAG;
        fpga_req->ies_board_model = FLASH_CONF_BRD_MODEL_DRS_VNS_6;
    } else if (0 == (error = cli_parse_word( cmd ,"1275e_ies12" ))) {
        fpga_req->board = FLASH_CONF_UNIT_MODEL_TAG;
        fpga_req->ies_board_model = FLASH_CONF_BRD_MODEL_IES_1275_12;
    } else if (0 == (error = cli_parse_word( cmd ,"ies8" ))) {
        fpga_req->board = FLASH_CONF_UNIT_MODEL_TAG;
        fpga_req->ies_board_model = FLASH_CONF_BRD_MODEL_DRS_VNS_8;
    } else if (0 == (error = cli_parse_word( cmd ,"ies16" ))) {
        fpga_req->board = FLASH_CONF_UNIT_MODEL_TAG;
        fpga_req->ies_board_model = FLASH_CONF_BRD_MODEL_DRS_VNS_16;
    } else if (0 == (error = cli_parse_word(cmd, "write"))) {
        fpga_req->board = IES_BOARDID_INTERCONNECT_BRD_REV;
    }
    else
    {
        fpga_req->set = FALSE;
        error = VTSS_INVALID_PARAMETER;
    }
    return (error);
}
static void cli_cmd_epe_license(cli_req_t *req)
{
    vns_fpga_req_t *fpga_req = req->module_req;
    vns_fpga_epe_license_info_t license_info;
    BOOL conf_mode=TRUE;

    conf_board_t conf;


    if (req->set) {
        if (conf_mgmt_board_get(&conf) < 0)
            return;
        license_info = fpga_req->license_info;
        /* if(is_epe_license_valid(&license_info)) */
        /* { */
            /*set_config_mode(&conf_mode);*/
            set_epe_license(license_info);
            save_vns_config();
            conf_mode=FALSE;
            /*set_config_mode(&conf_mode);*/
        /* } */

    } else 
    {
        get_epe_license(&license_info);
        CPRINTF("%08X %08X %08X %08X %08X\n", 
                license_info.reserved1, 
                license_info.reserved2, 
                license_info.ies_signature, 
                license_info.epe_signature, 
                license_info.key);
    }
}

static void cli_cmd_board_id_revision(cli_req_t *req)
{
    int i;
    uint id=0;
    /* ies_board_rev_info_t info; */
    vns_fpga_req_t *fpga_req = req->module_req;
    if (req->set) {
        if (fpga_req->set) {
            if(set_board_revision( fpga_req->board ,fpga_req->data_word))
            { CPRINTF("set_board_id failed\n"); }
        }
        else {
            if( get_board_revision( fpga_req->board, &id))
            { CPRINTF("get_board_id failed\n"); }
            else 
            {
                CPRINTF("%s: %d\n",flash_conf_tag[fpga_req->board].tag, id);
            }
        }
    } 
    else 
    {
        for(i=0; i< FLASH_CONF_END_TAG; i++)
        {
            if( get_board_revision( i, &id))
            { CPRINTF("get_board_id failed\n"); }
            else
            {
                if(strstr(flash_conf_tag[i].tag, "REV") != NULL)
                    CPRINTF("%s:\t\t %d\n",flash_conf_tag[i].tag, id);
            }
        }
    }
}


static void cli_cmd_ies_board_model(cli_req_t *req)
{
    uint id=0;
    /* ies_board_rev_info_t info; */
    vns_fpga_req_t *fpga_req = req->module_req;
    /* if (fpga_req->set) { */
    if (req->set) {
        if(set_board_revision( fpga_req->board ,fpga_req->ies_board_model))
        { CPRINTF("set_board_id failed\n"); }
    }
    else {
        if( get_board_revision( FLASH_CONF_UNIT_MODEL_TAG, &id))
        { CPRINTF("get_board_id failed\n"); }
        else {
            CPRINTF("%s: %s\n",flash_conf_tag[FLASH_CONF_UNIT_MODEL_TAG].tag, flash_conf_model[id].model);
        }
    }
}

static void cli_cmd_ies_board_id(cli_req_t *req)
{
    int i;
    uint id=0;
    /* ies_board_rev_info_t info; */
    vns_fpga_req_t *fpga_req = req->module_req;
    if (req->set) {
        if (fpga_req->set) {
            if(set_board_revision( fpga_req->board ,fpga_req->data_word))
            { CPRINTF("set_board_id failed\n"); }
        }
        else {
            if( get_board_revision( fpga_req->board, &id))
            { CPRINTF("get_board_id failed\n"); }
            else 
            {
                CPRINTF("%s: %d\n",flash_conf_tag[fpga_req->board].tag, id);
            }
        }
    } 
    else 
    {
        for(i=0; i< FLASH_CONF_END_TAG; i++)
        {
            if( get_board_revision( i, &id))
            { CPRINTF("get_board_id failed\n"); }
            else
            {
                if(strstr(flash_conf_tag[i].tag, "REV") != NULL)
                { /* do not want this */ }
                else if( strstr(flash_conf_tag[i].tag, "EPELIC") != NULL)
                { /* do not want this */ }
                else
                    if(strlen(flash_conf_tag[i].tag) > 8)
                        CPRINTF("%s:\t\t %d\n",flash_conf_tag[i].tag, id);
                    else
                        CPRINTF("%s:\t\t\t %d\n",flash_conf_tag[i].tag, id);
            }
        }
    }
}

static void cli_cmd_board_id(cli_req_t *req)
{
    u32 id=0;
    if (req->set) {

        if(set_board_id(req->value))
        { CPRINTF("set_board_id failed\n"); }
    } 
    else 
    {
        if( get_board_id(&id))
        { CPRINTF("get_board_id failed\n"); }
        else
        {
            CPRINTF("board id: 0x%08X\n", id);
            /* CPRINTF("Stack Copy: %s\n", cli_bool_txt(conf.stack_copy)); */
        }
    }
}


/* Debug Flash save mode */
static void cli_cmd_conf_mode(cli_req_t *req)
{
    /* cli_cmd_flash_conf(req, 1); */
    BOOL conf_mode; 

    if (req->set) {
        conf_mode= req->enable;
        if(set_config_mode(&conf_mode))
        { CPRINTF("conf_set failed\n"); }
    } 
    else 
    {
        if( get_config_mode(&conf_mode) )
        { CPRINTF("conf_get failed\n"); }
        else
        {
            CPRINTF("Flash Save: %s\n", cli_bool_txt(conf_mode));
            /* CPRINTF("Stack Copy: %s\n", cli_bool_txt(conf.stack_copy)); */
        }
    }
}
static void cli_cmd_epe_mirror(cli_req_t *req, BOOL port, BOOL sid, BOOL mode, BOOL include_stack_ports)
{
    switch_iter_t        sit;
    mirror_conf_t        conf;
    mirror_switch_conf_t switch_conf;
    char                 buf[80];
    vtss_port_no_t       iport_dis;
    mirror_cli_req_t     *mirror_req = req->module_req;
    vtss_rc              rc;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    mirror_mgmt_conf_get(&conf);

    /* Get/set mirror port */
    if (port || sid) {
        if (req->set) {
            if (port) {
                iport_dis = uport2iport(mirror_req->uport_dis);
                CPRINTF("Mirror User Port = %u, usid:%d", mirror_req->uport_dis, conf.mirror_switch);
#if VTSS_SWITCH_STACKABLE
                if (mirror_req->uport_dis && port_isid_port_no_is_stack(topo_usid2isid(conf.mirror_switch), iport_dis)) {
                    CPRINTF("Stack port %u for switch %d cannot be mirror port\n", mirror_req->uport_dis, conf.mirror_switch);
                    return;
                }
#endif
                conf.dst_port = iport_dis;
            }
#if VTSS_SWITCH_STACKABLE
            if (sid && vtss_stacking_enabled()) {
                if (msg_switch_exists(topo_usid2isid(req->usid[0]))) {
                    if (port_isid_port_no_is_stack(topo_usid2isid(req->usid[0]), conf.dst_port)) {
                        CPRINTF("Stack port %u for switch %d cannot be mirror port\n", conf.dst_port, req->usid[0]);
                    } else {
                        conf.mirror_switch = topo_usid2isid(req->usid[0]);
                    }
                } else {
                    CPRINTF("Invalid switch id:%d\n", req->usid[0]);
                }
            }
#endif

            if ((rc = mirror_mgmt_conf_set(&conf)) != VTSS_OK) {
                cli_printf("\n%s\n", error_txt(rc));
            }
        } else {
            if (port) {
                if (conf.dst_port != VTSS_PORT_NO_NONE) {
                    sprintf(buf, "%u", iport2uport(conf.dst_port));
                } else {
                    strcpy(buf, cli_bool_txt(0));
                }
                CPRINTF("Mirror Port: %s\n", buf);
            }
#if VTSS_SWITCH_STACKABLE
            if (sid && vtss_stacking_enabled()) {
                CPRINTF("Mirror SID : %d\n", topo_isid2usid(conf.mirror_switch));
            }
#endif
        }
    }

    if (!mode) {
        return;
    }

    /* Get/set mirror mode */
    (void)cli_switch_iter_init(&sit);
    while (cli_switch_iter_getnext(&sit, req)) {
        port_iter_t pit;

        // Get current configuration
        if ((rc = mirror_mgmt_switch_conf_get(sit.isid, &switch_conf)) != VTSS_OK) {
            cli_printf("\n%s\n", error_txt(rc));
            continue;
        }

        // Loop through all ports
        (void)cli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK);
        while (cli_port_iter_getnext(&pit, req)) {
            BOOL stack_port = port_isid_port_no_is_stack(sit.isid, pit.iport);

            if (req->set) {
                if (stack_port && !include_stack_ports) {
                    CPRINTF("Stack port %u for switch %d cannot be mirrored (ignored)\n", iport2uport(pit.iport), sit.usid);
                    continue; // Only allow stack ports if include_stack_ports is set
                }

                switch_conf.src_enable[pit.iport] = (req->enable || mirror_req->rx);
                switch_conf.dst_enable[pit.iport] = (req->enable || mirror_req->tx);

                CPRINTF(pit.iport, "src:%d, dst:%d, req_ena:%d, req_rx:%d, req:_tx:%d",
                         switch_conf.src_enable[pit.iport], switch_conf.dst_enable[pit.iport]
                         , req->enable, mirror_req->rx, mirror_req->tx);

                if (pit.iport == conf.dst_port && conf.mirror_switch == sit.isid && switch_conf.dst_enable[pit.iport] == TRUE) {
                    // Note: This is done in mirror.c when the configuration is updated.
                    CPRINTF("Setting Tx mirroring for mirror port (port %u) has no effect. Tx mirroring ignored \n", iport2uport(conf.dst_port));
                }
                continue;
            }
            if (pit.first) {
                cli_cmd_usid_print(sit.usid, req, 1);
                cli_table_header("Port  Mode      ");
            }

            if (!stack_port || include_stack_ports ||
                switch_conf.src_enable[pit.iport] ||  // Always show src and/or dst enabled ports - even stack ports
                switch_conf.dst_enable[pit.iport]) {
                CPRINTF("%-2u    %s %s\n",
                        pit.uport,
        switch_conf.src_enable[pit.iport] ? switch_conf.dst_enable[pit.iport] ? "Enabled" : "Rx" :
                        switch_conf.dst_enable[pit.iport] ? "Tx" : "Disabled",
                        stack_port ? "(Stack Port!)" : "");
            }
        }

        if (req->set) {
#ifdef VTSS_FEATURE_MIRROR_CPU
            // Set CPU mirroring
            if (req->cpu_port) {
                switch_conf.cpu_src_enable = (req->enable || mirror_req->rx);
                switch_conf.cpu_dst_enable = (req->enable || mirror_req->tx);
            }
#endif
            mirror_mgmt_switch_conf_set(sit.isid, &switch_conf);
        } else {
#ifdef VTSS_FEATURE_MIRROR_CPU
            // Set CPU mirroring
            if (req->cpu_port) {
                CPRINTF("%s   %s \n",
                        "CPU",
        switch_conf.cpu_src_enable ? switch_conf.cpu_dst_enable ? "Enabled" : "Rx" :
                        switch_conf.cpu_dst_enable ? "Tx" : "Disabled");
            }
#endif
        }
    }
}

static int32_t cli_epe_port_ch7_word_count (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MIN,
            max = VNS_EPE_CH7_MINOR_FRAME_WORD_COUNT_MAX;

    vns_fpga_req_t *fpga_req = req->module_req;

    req->parm_parsed = 1;
    /* error = (cli_parse_ulong(cmd, &value, min, max) && cli_parm_parse_keyword(cmd, cmd2, stx, cmd_org, req)); */
    error = cli_parse_ulong(cmd, &value, min, max); 
    if (error) 
        value = 1;
    fpga_req->epe_config.ch7_data_word_count = value;
    /* if (!error) ptp_req->ptp_instance_no = value; */
    return error;
}

static int32_t cli_discrete_out_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = 0, max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->discrete_out = value;
    return error;
}

static int32_t cli_flashing_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = 0, max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->led_flashing = value;
    return error;
}

static int32_t cli_green_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = 0, max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->led_green = value;
    return error;
}

static int32_t cli_red_blue_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = 0, max = 0xFFFFFFFF;
    vns_fpga_req_t *fpga_req = req->module_req;
    req->parm_parsed = 1;
    error = cli_parse_hex_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->led_red_blue = value;
    return error;
}

static int32_t cli_epe_port_bit_rate (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = VNS_EPE_BIT_RATE_MIN, max = VNS_EPE_BIT_RATE_MAX;
    /* ulong   min   = 0, max = 0xFFFFFFFF; */

    vns_fpga_req_t *fpga_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->epe_config.bit_rate = value;
    return error;
}

static int32_t cli_epe_port_dis (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;

    vns_fpga_req_t *fpga_req = req->module_req;

    /* req->parm_parsed = 1; */
    /* if (!error) fpga_req->epe_config.bit_rate = value; */
    error = cli_epe_parse_keyword(cmd, cmd2, stx, cmd_org, req);
    return error;
}

static int32_t cli_board_rev_keyword_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;

    vns_fpga_req_t *fpga_req = req->module_req;

    req->parm_parsed = 1;
    /* error = (cli_parse_ulong(cmd, &value, min, max) && cli_parm_parse_keyword(cmd, cmd2, stx, cmd_org, req)); */
    /* CPRINTF("char* parse %s %s %s %s \n", cmd, cmd2, stx, cmd_org); */

    error = cli_board_rev_parse_keyword(cmd, cmd2, stx, cmd_org, req);
    if (!error && cmd2 == NULL) fpga_req->set = FALSE;
    else fpga_req->set = TRUE;
    return error;
}

static int32_t cli_board_id_keyword_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;

    vns_fpga_req_t *fpga_req = req->module_req;

    req->parm_parsed = 1;
    /* error = (cli_parse_ulong(cmd, &value, min, max) && cli_parm_parse_keyword(cmd, cmd2, stx, cmd_org, req)); */
    /* CPRINTF("char* parse %s %s %s %s \n", cmd, cmd2, stx, cmd_org); */

    error = cli_board_id_parse_keyword(cmd, cmd2, stx, cmd_org, req);
    if (!error && cmd2 == NULL) fpga_req->set = FALSE;
    else fpga_req->set = TRUE;
    return error;
}

static int32_t cli_board_id_rev_param_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
        cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    ulong   min   = 0, max = 0xFFFFFFFF;

    vns_fpga_req_t *fpga_req = req->module_req;

    req->parm_parsed = 1;
    /* error = (cli_parse_ulong(cmd, &value, min, max) && cli_parm_parse_keyword(cmd, cmd2, stx, cmd_org, req)); */
    /* CPRINTF("char* parse %s %s %s %s \n", cmd, cmd2, stx, cmd_org); */

    error &= cli_parse_ulong(cmd, &value, min, max); 
    if (!error) fpga_req->data_word = value;

    if (error) {
        CPRINTF("ulong parse failed %s %s %d \n", cmd, cmd2, value);
    }
    /* if (!error) ptp_req->ptp_instance_no = value; */
    return error;
}

/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/

static cli_parm_t vns_fpga_cli_parm_table[] = {
    {
        "<red_blue>",
        "Change LED's Red & Blue. 0 - 0xFFFFFFFF.",
        CLI_PARM_FLAG_SET,
        cli_red_blue_parse,
        FPGA_dot_led_test
    },
    {
        "<green>",
        "Change LED's Green. 0 - 0xFFFFFFFF.",
        CLI_PARM_FLAG_SET,
        cli_green_parse,
        FPGA_dot_led_test
    },
    {
        "<flashing>",
        "Change LED's Flashing. 0 - 0xFFFFFFFF.",
        CLI_PARM_FLAG_SET,
        cli_flashing_parse,
        FPGA_dot_led_test
    },
    {
        "<discrete_out>",
        "Change Discrete outs 0 - 0xFFFFFFFF.",
        CLI_PARM_FLAG_SET,
        cli_discrete_out_parse,
        FPGA_dot_led_test
    },
    {
        "<integer>",
        "Board serial number ('xxxx'), default: Show serial number",
        CLI_PARM_FLAG_SET,
        cli_fpga_int_parse,
        FPGA_dot_led_test
    },
    {
        "<port>",
        "time to sleep",
        CLI_PARM_FLAG_SET,
        cli_fpga_epe_multi_parse,
        cli_cmd_epe_multi
    },
    {
        "<time>",
        "time to sleep",
        CLI_PARM_FLAG_SET,
        cli_fpga_epe_multi_parse,
        cli_cmd_epe_multi
    },
    {
        "<integer>",
        "Board serial number ('xxxx'), default: Show serial number",
        CLI_PARM_FLAG_SET,
        cli_fpga_int_parse,
        FPGA_dot_sn_cmd
    },
    {
        "<i2c_data_word>",
        "I2C data word",
        CLI_PARM_FLAG_SET,
        cli_fpga_i2c_data_word_parse,
        NULL
    },
    {
        "<i2c_reg>",
        "FPGA I2C register",
        CLI_PARM_FLAG_SET,
        cli_fpga_i2c_register_parse,
        NULL
    },
    {
        "<i2c_offset>",
        "FPGA I2C register",
        CLI_PARM_FLAG_SET,
        cli_fpga_i2c_offset_parse,
        NULL
    },
    {
        "in|out",
        "\tin   : direction in\n"
            "\tout  : direction out\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_direction_parse,
        NULL
    },
    {
        DOT_CMD_STR_DC1PPS_ARGS,
        DOT_CMD_STR_DC1PPS_DESC,
        CLI_PARM_FLAG_SET,
        cli_fpga_onoff_parse,
        NULL
    },
    {
        DOT_CMD_STR_GPS_ARGS,
        "Antenna DC Bias voltage.\n"
            "\t0     : Set Antenna DC Bias voltage to 0 volts (off)\n"
            "\t3     : Set Antenna DC Bias voltage to 3.3 volts\n"
            "\t5     : Set Antenna DC Bias voltage to 5 volts\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_035_parse,
        NULL
    },
    {
        "<feature>",
        "Health feature number for which to show detailed health information",
        CLI_PARM_FLAG_SET,
        cli_fpga_feature_parse,
        NULL
    },
    {
        "<disc_num>",
        "The discrete number to get or set.\n"
            "\tFor input discretes, this can be a value of 1-4\n"
            "\tFor output discretes, this can be a value of 1-12",
        CLI_PARM_FLAG_SET,
        cli_fpga_disc_num_parse,
        NULL
    },
    {
        DOT_CMD_STR_TIMIN_ARGS,
        DOT_CMD_STR_TIMIN_HELP_STR,
        CLI_PARM_FLAG_SET,
        cli_fpga_time_in_cfg_parse,
        NULL
    },
#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
    {
        "am|dc",
        "The time input signal setting.\n"
            "\tam:		Input signal is Amplitude Modulated\n"
            "\tdc:		Input signal is Digital.\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_am_dc_parse,
        NULL
    },
#endif
    {
        "none|a|b|g",
        "The IRIG time output setting.\n"
            "\tnone:	IRIG time output is disabled.\n"
            "\ta:		IRIG-A time code.\n"
            "\tb:		IRIG-B time code.\n"
            "\tg:		IRIG-G time code.",
        CLI_PARM_FLAG_SET,
        cli_fpga_time_out_cfg_parse,
        NULL
    },
    {
        "<disc_setting>",
        DOT_CMD_STR_DISCRETE_HELP_STR,
        CLI_PARM_FLAG_SET,
        cli_fpga_disc_setting_parse,
        NULL
    },
    {
        "<cal_3>",
        "Moves the time signal/stamp by the largest amount.\n"
            "\tNot valid for time input...\n"
            "\tFor Time Output, moves the entire signal 10 sine waves. Valid values: 0-63\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_cal_num_parse,
        NULL
    },
    {
        "<cal_2>",
        "Moves the time signal/stamp by the second largest amount.\n"
            "\tNot valid for time input...\n"
            "\tFor Time Output, moves the entire signal 1 sine waves. Valid values: 0-9\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_cal_num_parse,
        NULL
    },
    {
        "<cal_1>",
        "Moves the time signal/stamp by the smallest amount... This is the only cal for time input.\n"
            "\tFor Time Input, moves the time stamp by 0.1 microseconds.\n"
            "\t\tValid values: 0 - 0x8191.\n"
            "\tFor Time Output, moves the entire signal 0.1 microseconds."
            "\t\tValid values A: 0 - 0x3E7, \n"
            "\t\tValid values B: 0 - 0x270F \n"
            "\t\tValid values G: 0 - 0x63, \n",
        CLI_PARM_FLAG_SET,
        cli_fpga_cal_num_parse,
        NULL
    },
    {
        "<value>",
        DOT_CMD_STR_DISOUT_HELP_STR,
        CLI_PARM_FLAG_SET,
        cli_fpga_disc_val_parse,
        NULL
    },
    {
        DOT_CMD_STR_1588_ARG_MD_CLK,
            "\tdisable       : removes PTP clock instance 0\n"
            "\t\tmaster|slave  : creates PTP clock instance 0 with a default profile\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_modify_clk_parse,
        FPGA_dot_1588_cmd
    },
    {
        DOT_CMD_STR_1588_ARG_MSTR_SLV ,
        DOT_CMD_STR_1588_DESC "\n",
        CLI_PARM_FLAG_SET,
        cli_fpga_1588_mode_parse,
        FPGA_dot_1588_cmd
    },
#ifdef IES_EPE_FEATURE_ENABLE
    {
        "<bit_rate>",
        "bitrate: The bitrate is set as " STR(VNS_EPE_BIT_RATE_MIN) "-" STR(VNS_EPE_BIT_RATE_MAX) " bps ",
        CLI_PARM_FLAG_SET,
        cli_epe_port_bit_rate,
        cli_cmd_epe_port
    },
    {
        "encode|decode",
        "Selects PCM state",
        CLI_PARM_FLAG_SET,
        cli_epe_port_dis,
        cli_cmd_epe_port
    },
    {
        "<ch7_word_count>",
        "Sets the number of data words in a minor frame "
            "valid numbers are 1-8",
        CLI_PARM_FLAG_SET,
        cli_epe_port_ch7_word_count,
        cli_cmd_epe_port
    },

    {
        "hdlc|ch7|disable",
        "Configure port for HDLC or Chapter 7-15 default: HDLC",
        CLI_PARM_FLAG_SET,
        cli_epe_port_dis,
        cli_cmd_epe_port
    },
    {
        "invert_clock",
        "Selects PCM Clockstate",
        CLI_PARM_FLAG_SET,
        cli_epe_port_dis,
        cli_cmd_epe_port
    },
    {
        "remove_fcs",
        "Selects PCM Clockstate",
        CLI_PARM_FLAG_SET,
        cli_epe_port_dis,
        cli_cmd_epe_port
    },
#endif /* IES_EPE_FEATURE_ENABLE */
    {
        IES_DEBUG_CMD_STR_BRD_MDL_ARGS,
        "Configure board model number",
        CLI_PARM_FLAG_SET,
        cli_board_id_keyword_parse,
        cli_cmd_ies_board_model
    },
    {
        "power|fpga|switch|interconnect|unit_sn",
        "Configure board id with board revisions",
        CLI_PARM_FLAG_SET,
        cli_board_id_keyword_parse,
        cli_cmd_ies_board_id
    },
    {
        "power|fpga|switch|interconnect",
        "Configure board id with board revisions",
        CLI_PARM_FLAG_SET,
        cli_board_rev_keyword_parse,
        cli_cmd_board_id_revision
    },
    {
        "<board_id>",
        "uint ",
        CLI_PARM_FLAG_SET,
        cli_board_id_rev_param_parse,
        cli_cmd_ies_board_id
    },
    {
        "<revision>",
        "0x0 - 0xFF",
        CLI_PARM_FLAG_SET,
        cli_board_id_rev_param_parse,
        cli_cmd_board_id_revision
    },
    {
        "enable|disable",
        "enable  : Enable Flash saving\n"
            "disable : Disable Flash saving\n"
            "(default: Show Flash save mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        cli_cmd_conf_mode
    },
    {
        "<sect_a>",
        "License",
        CLI_PARM_FLAG_SET,
        cli_param_epe_license_section_a,
        cli_cmd_epe_license
    },
    {
        "<sect_b>",
        "License",
        CLI_PARM_FLAG_SET,
        cli_param_epe_license_section_b,
        cli_cmd_epe_license
    },
    {
        "<sect_c>",
        "License",
        CLI_PARM_FLAG_SET,
        cli_param_epe_license_section_c,
        cli_cmd_epe_license
    },
    {
        "<sect_d>",
        "License",
        CLI_PARM_FLAG_SET,
        cli_param_epe_license_section_d,
        cli_cmd_epe_license
    },
    {
        "<sect_e>",
        "License",
        CLI_PARM_FLAG_SET,
        cli_param_epe_license_section_e,
        cli_cmd_epe_license
    },
    {
        "<hex_ulong>",
        "32 bit in length",
        CLI_PARM_FLAG_SET,
        cli_param_hex_ulong,
        cli_cmd_board_id
    },
    {
        "<hex_feature_set>",
        "32 bit in length",
        CLI_PARM_FLAG_SET,
        cli_param_hex_ulong,
        cli_cmd_board_id
    },
    {
        "<date_time>",
        "YYYY-jjj-hh:mm:ss",
        CLI_PARM_FLAG_SET,
        cli_param_date_time,
        FPGA_dot_time_cmd
    },
    {
        "<+/-seconds>",
        "Add or subtract seconds. Seconds defaults to 1",
        CLI_PARM_FLAG_SET,
        cli_param_date_time,
        FPGA_dot_time_cmd
    },
    {NULL, NULL, 0, 0, NULL}

};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/
ADD_CLI_TAB_ENTRIES();
#if 0 /* COMMENT_OUT */
#if !defined(BOARD_IGU_REF)

cli_cmd_tab_entry (
        NULL,
        "Debug IES EPE LICENSE [<sect_a>] [<sect_b>] [<sect_c>] [<sect_d>] [<sect_e>]",
        "Sets license for Ethernet PCM encder",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        cli_cmd_epe_license,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug IES BOARD MODEL [ies12|ies6|1275e_ies12]",
        "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        cli_cmd_ies_board_model,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug IES BOARD NUMBER [power|fpga|switch|interconnect|unit_sn] [<board_id>]",
        "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        cli_cmd_ies_board_id,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug IES BOARD REVISION [power|fpga|switch|interconnect] [<revision>]",
        "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        cli_cmd_board_id_revision,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug IES BOARD ID SET [<hex_feature_set>]",
        "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        cli_cmd_board_id,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug IES set product",
        "Gets IES product or Sets product id in MAC address of switch \n\t 1 for iES-6, 2 for iES-12 ",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF, VTSS_MODULE_ID_VNS_FPGA, 
        FPGA_set_product_id_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".SN [<integer>]",
        "Gets serial number of switch board or Sets MAC address one time when unit is being commissioned",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_sn_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        "Debug .READ <i2c_reg> <i2c_offset>",
        NULL,
        "Debug FPGA I2C_Read Data Word",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_STATUS,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_i2c_debug_read,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug .WRITE <i2c_reg> <i2c_offset> <i2c_data_word> ",
        "Debug FPGA I2C_Write Data Word",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_i2c_debug_write,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug .DEBUG [on|off]",
        "Debug FPGA I2C_Write Data Word",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_debug,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        "Debug .timer [on|off]",
        "Debug FPGA I2C_Write Data Word",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_timer,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

#endif //!defined(BOARD_IGU_REF)

//note: not in debug group
cli_cmd_tab_entry (
        NULL,
        ".BIT",
        "FPGA Built In Test command",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_bit_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".CONFIG",
        "Show FPGA Configuration.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_config_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

#if !defined(BOARD_IGU_REF)

cli_cmd_tab_entry (
        NULL,
        ".DISCRETE [in|out] [<disc_num>] [<disc_setting>]",
        "FPGA Discrete command.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_discrete_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

#endif // !defined(BOARD_IGU_REF)

#if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
cli_cmd_tab_entry (
        NULL,
        ".GPS [0|3|5]",
        "Displays or sets the GPS Configuration.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_gps_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif /* BOARD_VNS_12_REF */
cli_cmd_tab_entry (
        NULL,
        ".HEALTH [<feature>]",
        "Displays FPGA Health Information.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_health_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".HELP",
        "Displays all dot commands.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_help_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".SAVE",
        "Save FPGA Configuration.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_save_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".STATUS",
        "FPGA Status command",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_status_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".RESET",
        "Resets the switch.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_reset_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
        ".TIME [<date_time>] [<+/-seconds>]",
        /* ".TIME [<date_time>]", */
        "Shows the system time.",
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_time_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

cli_cmd_tab_entry (
        NULL,
#if HIDE_IRIG_INPUTS
 #if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
        ".TIMIN [none|gps|1588]",
 #else
        ".TIMIN [none|1588]",
 #endif
        "Gets or sets the time in configuration.",
#else
        /* showing IRIG */
 #if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
        ".TIMIN [none|a|b|g|gps|1588|1pps] [am|dc]",
 #else
        ".TIMIN [none|a|b|g|1588]",
 #endif
 #if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
        "Gets or sets the time in configuration."
        "IRIG Input is only available if hardware permits",
 #else
        "Gets or sets the time in configuration.",
 #endif
#endif
        CLI_CMD_SORT_KEY_DEFAULT,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VNS_FPGA,
        FPGA_dot_time_in_cmd,
        vns_fpga_cli_req_default_set,
        vns_fpga_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );

#if !defined(BOARD_IGU_REF)

        cli_cmd_tab_entry (
                NULL,
                ".TIMOUT [none|a|b|g]",
                "Gets or sets the time out configuration.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_time_out_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

        cli_cmd_tab_entry (
                NULL,
                ".DC1PPS [on|off]",
                "Gets or sets the IRIG-DC output type: on = 1PPS, off = IRIG-DC.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_dc_1pps_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

        cli_cmd_tab_entry (
                NULL,
                ".TCAL [in|out] [<cal_1>] [<cal_2>] [<cal_3>]",
                "Gets or sets the time input or output calibrations.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_tcal_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

#endif // !defined(BOARD_IGU_REF)

        cli_cmd_tab_entry (
                NULL,
#if HIDE_IRIG_INPUTS
 #if defined (BOARD_VNS_12_REF) || defined (BOARD_VNS_16_REF)
                ".1588 [master|slave|disable] [modify_clock]",
 #else
                ".1588 [slave|disable] [modify_clock]",
 #endif
#else
                ".1588 [master|slave|disable] [modify_clock]",
#endif
                "Gets or sets the IEEE-1588 Mode for the FPGA.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_1588_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

        cli_cmd_tab_entry (
                NULL,
                ".VERSION",
                "Gets the current VNS type and version.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_version_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

        cli_cmd_tab_entry (
                NULL,
                "Debug .CONFIG",
                "Gets the current FPGA configuration.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_debug_config_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

        cli_cmd_tab_entry (
                NULL,
                ".INFO",
                "Gets iES product information.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_info_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );
        cli_cmd_tab_entry (
                NULL,
                "Debug .REGDUMP",
                "Gets the current FPGA configuration.",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_debug_regdump_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );

#if !defined(BOARD_IGU_REF)
        cli_cmd_tab_entry (
                NULL,
                ".DISOUT [<value>]",
                "Sets discrete outputs defined as USER",
                CLI_CMD_SORT_KEY_DEFAULT,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_VNS_FPGA,
                FPGA_dot_disout_cmd,
                vns_fpga_cli_req_default_set,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );
#endif

#ifdef IES_EPE_FEATURE_ENABLE
        cli_cmd_tab_entry (
                ".EPE Port",
                ".EPE Port [encode|decode|disable] [hdlc|ch7] [<bit_rate>] [<ch7_word_count>]",
                "Set or show the EPE port",
                VTSS_MODULE_ID_VNS_FPGA,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_MIRROR,
                cli_cmd_epe_port,
                NULL,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );
#endif /* IES_EPE_FEATURE_ENABLE */
        cli_cmd_tab_entry (
                NULL,
                "Debug IES EPE PORT [hdlc|ch7] [<bit_rate>] [<ch7_word_count>]",
                "Set or show the EPE port",
                VTSS_MODULE_ID_VNS_FPGA,
                CLI_CMD_TYPE_CONF,
                VTSS_MODULE_ID_MIRROR,
                cli_cmd_epe_debug,
                NULL,
                vns_fpga_cli_parm_table,
                CLI_CMD_FLAG_NONE
                );


        /* .EPE Mode is implimented in ../mirror/mirror_cli.c */
        /* It is the same as mirror mode with a different name*/
        //cli_cmd_tab_entry (
        //    ".EPE Mode [<port_list>]",
        //    ".EPE Mode [<port_list>] [enable|disable|rx|tx]",
        //    "Set or show the mirror mode",
        //    PRIO_MIRROR_MODE,
        //    CLI_CMD_TYPE_CONF,
        //    VTSS_MODULE_ID_MIRROR,
        //    cli_cmd_epe_mode,
        //    NULL,
        //    mirror_cli_parm_table,
        //    CLI_CMD_FLAG_NONE
        //);
#endif /* IES_EPE_FEATURE_ENABLE */
