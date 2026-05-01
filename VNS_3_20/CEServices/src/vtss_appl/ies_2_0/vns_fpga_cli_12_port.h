/*
 * 
 *
 *  Created on: Oct 31, 2022
 *      Author: Jason Alden
 *      Copyright: Telspan Data 2022
 */


#ifndef VNS_FPGA_CLI_12_PORT_H_
#define VNS_FPGA_CLI_12_PORT_H_
#if defined (BOARD_VNS_12_REF)

#include "dot_command_strings.h" 


#undef DOT_CMD_STR_TIMIN_ARGS

#if HIDE_IRIG_INPUTS 
  #define DOT_CMD_STR_TIMIN_ARGS      "none|gps_0v|gps_3v|gps_5v|1588" 

#else 
  /* .TIMIN REDEFINE
   */
  /* #define DOT_CMD_STR_TIMIN_ARGS      "none|a|b|g|gps_0v|gps_3v|gps_5v|1588" */
  #define DOT_CMD_STR_TIMIN_ARGS      "none|a_dc|b_dc|g_dc|a_am|b_am|g_am|gps_0v|gps_3v|gps_5v|1588"
  
  /* .TIME REDEFINE
   */
  #undef DOT_CMD_STR_TIME_ARGS
  #define DOT_CMD_STR_TIME_ARG1       "<date_time>" 
  #define DOT_CMD_STR_TIME_ARG2       "<+/-seconds>" 
  #define DOT_CMD_STR_TIME_ARGS       SQ_BKT( DOT_CMD_STR_TIME_ARG1 ) " " SQ_BKT( DOT_CMD_STR_TIME_ARG2 )
  #undef DOT_CMD_STR_TIME_CMD
  #define DOT_CMD_STR_TIME_CMD        DOT_CMD_STR_TIME " " DOT_CMD_STR_TIME_ARGS


#endif

#undef DOT_CMD_STR_TIMIN_CMD
#define DOT_CMD_STR_TIMIN_CMD        DOT_CMD_STR_TIMIN " " SQ_BKT( DOT_CMD_STR_TIMIN_ARGS )

#undef DOT_CMD_STR_HELP_FUNC
#define DOT_CMD_STR_HELP_FUNC FPGA_dot_ies_12_help_cmd


#define VNS_FPGA_HELP_STRING     \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_1588,        DOT_CMD_STR_1588_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_BIT,         DOT_CMD_STR_BIT_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_CONFIG,      DOT_CMD_STR_CONFIG_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DC1PPS,      DOT_CMD_STR_DC1PPS_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISCRETE,    DOT_CMD_STR_DISCRETE_DESC) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISOUT,      DOT_CMD_STR_DISOUT_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_GPS,         DOT_CMD_STR_GPS_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_EPE,         DOT_CMD_STR_EPE_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_EPE_MULTI,   DOT_CMD_STR_EPE_MULTI_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_EPE_ENC_STAT, DOT_CMD_STR_EPE_ENC_STAT_DESC) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_EPE_DEC_STAT, DOT_CMD_STR_EPE_DEC_STAT_ARGS) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_HEALTH,      DOT_CMD_STR_HEALTH_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_HELP,        DOT_CMD_STR_HELP_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_INFO,        DOT_CMD_STR_INFO_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_RESET,       DOT_CMD_STR_RESET_DESC   ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_SAVE,        DOT_CMD_STR_SAVE_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_STATUS,      DOT_CMD_STR_STATUS_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_TIME,        DOT_CMD_STR_TIME_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_TIMIN,       DOT_CMD_STR_TIMIN_DESC   ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_TIMOUT,      DOT_CMD_STR_TIMOUT_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_TCAL,        DOT_CMD_STR_TCAL_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_VERSION,     DOT_CMD_STR_VERSION_DESC ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_SN,          DOT_CMD_STR_SN_DESC      )

#define ADD_CLI_TAB_ENTRIES() \
        /***************************************************************************/ \
        /*                          iES DOT Commands                               */ \
        /***************************************************************************/ \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_1588_CMD,           DOT_CMD_STR_1588_DESC,        DOT_CMD_STR_1588_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_BIT_CMD,            DOT_CMD_STR_BIT_DESC,         DOT_CMD_STR_BIT_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_CONFIG_CMD,         DOT_CMD_STR_CONFIG_DESC,      DOT_CMD_STR_CONFIG_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_DC1PPS_CMD,         DOT_CMD_STR_DC1PPS_DESC,      DOT_CMD_STR_DC1PPS_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_DISCRETE_CMD,       DOT_CMD_STR_DISCRETE_DESC,    DOT_CMD_STR_DISCRETE_FUNC) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_DISOUT_CMD,         DOT_CMD_STR_DISOUT_DESC,      DOT_CMD_STR_DISOUT_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_GPS_CMD,            DOT_CMD_STR_GPS_DESC,         DOT_CMD_STR_GPS_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_EPE_CMD,            DOT_CMD_STR_EPE_DESC,         DOT_CMD_STR_EPE_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_EPE_MULTI_CMD,      DOT_CMD_STR_EPE_MULTI_DESC,   DOT_CMD_STR_EPE_MULTI_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_EPE_ENC_STAT_CMD,   DOT_CMD_STR_EPE_ENC_STAT_DESC,DOT_CMD_STR_EPE_ENC_STAT_FUNC) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_EPE_DEC_STAT_CMD,   DOT_CMD_STR_EPE_DEC_STAT_DESC,DOT_CMD_STR_EPE_DEC_STAT_FUNC) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_HEALTH_CMD,         DOT_CMD_STR_HEALTH_DESC,      DOT_CMD_STR_HEALTH_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_HELP_CMD,           DOT_CMD_STR_HELP_DESC,        DOT_CMD_STR_HELP_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_INFO_CMD,           DOT_CMD_STR_INFO_DESC,        DOT_CMD_STR_INFO_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_RESET_CMD,          DOT_CMD_STR_RESET_DESC,       DOT_CMD_STR_RESET_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_SAVE_CMD,           DOT_CMD_STR_SAVE_DESC,        DOT_CMD_STR_SAVE_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_STATUS_CMD,         DOT_CMD_STR_STATUS_DESC,      DOT_CMD_STR_STATUS_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_TIME_CMD,           DOT_CMD_STR_TIME_DESC,        DOT_CMD_STR_TIME_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_TIMIN_CMD,          DOT_CMD_STR_TIMIN_DESC,       DOT_CMD_STR_TIMIN_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_TIMOUT_CMD,         DOT_CMD_STR_TIMOUT_DESC,      DOT_CMD_STR_TIMOUT_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_TCAL_CMD,           DOT_CMD_STR_TCAL_DESC,        DOT_CMD_STR_TCAL_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_VERSION_CMD,        DOT_CMD_STR_VERSION_DESC,     DOT_CMD_STR_VERSION_FUNC ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  DOT_CMD_STR_SN_CMD,             DOT_CMD_STR_SN_DESC,          DOT_CMD_STR_SN_FUNC      ) \
        /***************************************************************************/ \
        /*                          iES Debug Commands                             */ \
        /***************************************************************************/ \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_EPE_LIC_CMD,        IES_DEBUG_CMD_STR_EPE_LIC_DESC,     IES_DEBUG_CMD_STR_EPE_LIC_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_BRD_MDL_CMD,        IES_DEBUG_CMD_STR_BRD_MDL_DESC,     IES_DEBUG_CMD_STR_BRD_MDL_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_BRD_NUM_CMD,        IES_DEBUG_CMD_STR_BRD_NUM_DESC,     IES_DEBUG_CMD_STR_BRD_NUM_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_BRD_REV_CMD,        IES_DEBUG_CMD_STR_BRD_REV_DESC,     IES_DEBUG_CMD_STR_BRD_REV_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_BRD_ID_CMD,         IES_DEBUG_CMD_STR_BRD_ID_DESC,      IES_DEBUG_CMD_STR_BRD_ID_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_PRDT_ID_CMD,        IES_DEBUG_CMD_STR_PRDT_ID_DESC,     IES_DEBUG_CMD_STR_PRDT_ID_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_I2C_READ_CMD,       IES_DEBUG_CMD_STR_I2C_READ_DESC,    IES_DEBUG_CMD_STR_I2C_READ_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_I2C_WRITE_CMD,      IES_DEBUG_CMD_STR_I2C_WRITE_DESC,   IES_DEBUG_CMD_STR_I2C_WRITE_FUNC ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_DEBUG_CMD,          IES_DEBUG_CMD_STR_DEBUG_DESC,       IES_DEBUG_CMD_STR_DEBUG_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_TIMER_CMD,          IES_DEBUG_CMD_STR_TIMER_DESC,       IES_DEBUG_CMD_STR_TIMER_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_LED_TEST_CMD,       IES_DEBUG_CMD_STR_LED_TEST_DESC,    IES_DEBUG_CMD_STR_LED_TEST_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_CONFIG_CMD,         IES_DEBUG_CMD_STR_CONFIG_DESC,      IES_DEBUG_CMD_STR_CONFIG_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_REGDUMP_CMD,        IES_DEBUG_CMD_STR_REGDUMP_DESC,     IES_DEBUG_CMD_STR_REGDUMP_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_EPE_PORT_CMD,       IES_DEBUG_CMD_STR_EPE_PORT_DESC,    IES_DEBUG_CMD_STR_EPE_PORT_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_TIME_IN_REG_CMD,    IES_DEBUG_CMD_STR_TIME_IN_REG_DESC, IES_DEBUG_CMD_STR_TIME_IN_REG_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_ENCODER_REG_CMD,    IES_DEBUG_CMD_STR_ENCODER_REG_DESC, IES_DEBUG_CMD_STR_ENCODER_REG_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( VNS_NULL,  IES_DEBUG_CMD_STR_DECODER_REG_CMD,    IES_DEBUG_CMD_STR_DECODER_REG_DESC, IES_DEBUG_CMD_STR_DECODER_REG_FUNC  )


#define DOT_CMD_STR_TIMIN_HELP_STR \
            "The time input setting.\n" \
            "\tnone:	use switch time (no external source)\n" \
            "\t*IRIG inputs my not be available depending on hardware configuration.\n" \
            "\t To determine if your hardware is compatable, please run the command.\n" \
            "\ta:		use IRIG-A time code as master time source.\n" \
            "\tb:		use IRIG-B time code as master time source.\n" \
            "\tg:		use IRIG-G time code as master time source.\n" \
            "\tgps:		use gps as master time source\n" \
            "\t*1PPS input my not be available depending on hardware configuration.\n" \
            "\t To determine if your hardware is compatable, please run the command.\n" \
            "\t1588:	use IEEE-1588 time as master time source."

#define DOT_CMD_STR_DISCRETE_HELP_STR \
        "Sets the meaning of the discrete input/output (specified by in|out <disc_num>).\n" \
            " For input discretes:\n" \
            "\t0 :  disable\n" \
            " For output discretes:\n" \
            "\t0  : Port 1 Activity\n" \
            "\t1  : Port 2 Activity\n" \
            "\t2  : Port 3 Activity\n" \
            "\t3  : Port 4 Activity\n" \
            "\t4  : Port 5 Activity\n" \
            "\t5  : Port 6 Activity\n" \
            "\t6  : Port 7 Activity\n" \
            "\t7  : Port 8 Activity\n" \
            "\t8  : Port 9 Activity\n" \
            "\t9  : Port 10 Activity\n" \
            "\t10 : Port 11 Activity\n" \
            "\t11 : Port 12 Activity\n" \
            "\t12 : Port 1 Link Up\n" \
            "\t13 : Port 2 Link Up\n" \
            "\t14 : Port 3 Link Up\n" \
            "\t15 : Port 4 Link Up\n" \
            "\t16 : Port 5 Link Up\n" \
            "\t17 : Port 6 Link Up\n" \
            "\t18 : Port 7 Link Up\n" \
            "\t19 : Port 8 Link Up\n" \
            "\t20 : Port 9 Link Up\n" \
            "\t21 : Port 10 Link Up\n" \
            "\t22 : Port 11 Link Up\n" \
            "\t23 : Port 12 Link Up\n" \
            "\t24 : Time Lock\n" \
            "\t25 : 1588 Lock\n" \
            "\t26 : GPS Lock\n" \
            "\t27 : Switch Error\n" \
            "\t28 : FPGA Error\n" \
            "\t29 : User Control\n" \
            "\t30  : disable\n"

#define DOT_CMD_STR_DISOUT_HELP_STR \
        "Accepts a hexadecimal value and sets discrete outputs on or off if the output is configured to be under USER CONTROL.\n" \
            "The minimum value being 0 and maximum being FFF.\n" \
            "See the .DISCRETE for more details on how to configure for USER CONTROL.\n"

#else 
#error "Not including help file"
#endif /*VNS_FPGA_CLI_12_PORT_H_*/
#endif /*BOARD_VNS_12_REF*/
