/*
 * 
 *
 *  Created on: Oct 31, 2022
 *      Author: Jason Alden
 *      Copyright: Telspan Data 2022
 */


#ifndef VNS_FPGA_CLI_6_PORT_H_
#define VNS_FPGA_CLI_6_PORT_H_

#if defined(BOARD_VNS_6_REF)

#include "cli_api.h"        /* for cli_xxx()                                            */
#include "cli.h"            /* for cli_req_t and cli_parse_integer                      */
#include "dot_command_strings.h"

#undef DOT_CMD_STR_1588_ARGS
#undef DOT_CMD_STR_TIMIN_ARGS
#if HIDE_IRIG_INPUTS 
#define DOT_CMD_STR_1588_ARGS       "slave|disable [modify_clock]" 
#define DOT_CMD_STR_TIMIN_ARGS      "[none|1588] [modify_clock]" 
#else 
#define DOT_CMD_STR_1588_ARGS       "master|slave|disable [modify_clock]" 
#define DOT_CMD_STR_TIMIN_ARGS      "[none|a|b|g|1588] [modify_clock]" 
#endif

#undef DOT_CMD_STR_HELP_FUNC
#define DOT_CMD_STR_HELP_FUNC FPGA_dot_ies_6_help_cmd

#define VNS_FPGA_HELP_STRING     \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_1588,        DOT_CMD_STR_1588_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_BIT,         DOT_CMD_STR_BIT_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_CONFIG,      DOT_CMD_STR_CONFIG_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DC1PPS,      DOT_CMD_STR_DC1PPS_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISCRETE,    DOT_CMD_STR_DISCRETE_DESC) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISOUT,      DOT_CMD_STR_DISOUT_DESC  ) \
\
\
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
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_1588,      DOT_CMD_STR_1588_ARGS,        DOT_CMD_STR_1588_DESC,        DOT_CMD_STR_1588_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_BIT,       DOT_CMD_STR_BIT_ARGS,         DOT_CMD_STR_BIT_DESC,         DOT_CMD_STR_BIT_FUNC      ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_CONFIG,    DOT_CMD_STR_CONFIG_ARGS,      DOT_CMD_STR_CONFIG_DESC,      DOT_CMD_STR_CONFIG_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DC1PPS,    DOT_CMD_STR_DC1PPS_ARGS,      DOT_CMD_STR_DC1PPS_DESC,      DOT_CMD_STR_DC1PPS_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DISCRETE,  DOT_CMD_STR_DISCRETE_ARGS,    DOT_CMD_STR_DISCRETE_DESC,    DOT_CMD_STR_DISCRETE_FUNC ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DISOUT,    DOT_CMD_STR_DISOUT_ARGS,      DOT_CMD_STR_DISOUT_DESC,      DOT_CMD_STR_DISOUT_FUNC   ) \
\
\
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_HELP,      DOT_CMD_STR_HELP_ARGS,        DOT_CMD_STR_HELP_DESC,        DOT_CMD_STR_HELP_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_INFO,      DOT_CMD_STR_INFO_ARGS,        DOT_CMD_STR_INFO_DESC,        DOT_CMD_STR_INFO_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_RESET,     DOT_CMD_STR_RESET_ARGS,       DOT_CMD_STR_RESET_DESC,       DOT_CMD_STR_RESET_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_SAVE,      DOT_CMD_STR_SAVE_ARGS,        DOT_CMD_STR_SAVE_DESC,        DOT_CMD_STR_SAVE_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_STATUS,    DOT_CMD_STR_STATUS_ARGS,      DOT_CMD_STR_STATUS_DESC,      DOT_CMD_STR_STATUS_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIME,      DOT_CMD_STR_TIME_ARGS,        DOT_CMD_STR_TIME_DESC,        DOT_CMD_STR_TIME_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIMIN,     DOT_CMD_STR_TIMIN_ARGS,       DOT_CMD_STR_TIMIN_DESC,       DOT_CMD_STR_TIMIN_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIMOUT,    DOT_CMD_STR_TIMOUT_ARGS,      DOT_CMD_STR_TIMOUT_DESC,      DOT_CMD_STR_TIMOUT_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TCAL,      DOT_CMD_STR_TCAL_ARGS,        DOT_CMD_STR_TCAL_DESC,        DOT_CMD_STR_TCAL_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_VERSION,   DOT_CMD_STR_VERSION_ARGS,     DOT_CMD_STR_VERSION_DESC,     DOT_CMD_STR_VERSION_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_SN,        DOT_CMD_STR_SN_ARGS,          DOT_CMD_STR_SN_DESC,          DOT_CMD_STR_SN_FUNC       ) \
        /***************************************************************************/ \
        /*                          iES Debug Commands                             */ \
        /***************************************************************************/ \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_EPE_LIC,      IES_DEBUG_CMD_STR_EPE_LIC_ARGS,     IES_DEBUG_CMD_STR_EPE_LIC_DESC,     IES_DEBUG_CMD_STR_EPE_LIC_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_BRD_MDL,      IES_DEBUG_CMD_STR_BRD_MDL_ARGS,     IES_DEBUG_CMD_STR_BRD_MDL_DESC,     IES_DEBUG_CMD_STR_BRD_MDL_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_BRD_NUM,      IES_DEBUG_CMD_STR_BRD_NUM_ARGS,     IES_DEBUG_CMD_STR_BRD_NUM_DESC,     IES_DEBUG_CMD_STR_BRD_NUM_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_BRD_REV,      IES_DEBUG_CMD_STR_BRD_REV_ARGS,     IES_DEBUG_CMD_STR_BRD_REV_DESC,     IES_DEBUG_CMD_STR_BRD_REV_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_BRD_ID,       IES_DEBUG_CMD_STR_BRD_ID_ARGS,      IES_DEBUG_CMD_STR_BRD_ID_DESC,      IES_DEBUG_CMD_STR_BRD_ID_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_PRDT_ID,      IES_DEBUG_CMD_STR_PRDT_ID_ARGS,     IES_DEBUG_CMD_STR_PRDT_ID_DESC,     IES_DEBUG_CMD_STR_PRDT_ID_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_I2C_READ,     IES_DEBUG_CMD_STR_I2C_READ_ARGS,    IES_DEBUG_CMD_STR_I2C_READ_DESC,    IES_DEBUG_CMD_STR_I2C_READ_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_I2C_WRITE,    IES_DEBUG_CMD_STR_I2C_WRITE_ARGS,   IES_DEBUG_CMD_STR_I2C_WRITE_DESC,   IES_DEBUG_CMD_STR_I2C_WRITE_FUNC ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_DEBUG,        IES_DEBUG_CMD_STR_DEBUG_ARGS,       IES_DEBUG_CMD_STR_DEBUG_DESC,       IES_DEBUG_CMD_STR_DEBUG_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_TIMER,        IES_DEBUG_CMD_STR_TIMER_ARGS,       IES_DEBUG_CMD_STR_TIMER_DESC,       IES_DEBUG_CMD_STR_TIMER_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_CONFIG,       IES_DEBUG_CMD_STR_CONFIG_ARGS,      IES_DEBUG_CMD_STR_CONFIG_DESC,      IES_DEBUG_CMD_STR_CONFIG_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_REGDUMP,      IES_DEBUG_CMD_STR_REGDUMP_ARGS,     IES_DEBUG_CMD_STR_REGDUMP_DESC,     IES_DEBUG_CMD_STR_REGDUMP_FUNC   )

        /* CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_EPE_PORT,     IES_DEBUG_CMD_STR_EPE_PORT_ARGS,    IES_DEBUG_CMD_STR_EPE_PORT_DESC,    IES_DEBUG_CMD_STR_EPE_PORT_FUNC ) \ */


#define DOT_CMD_STR_TIMIN_HELP_STR \
        "The time input setting.\n" \
            "\tnone:	use switch time (no external source)\n" \
            "\ta:		use IRIG-A time code as master time source.\n" \
            "\tb:		use IRIG-B time code as master time source.\n" \
            "\tg:		use IRIG-G time code as master time source.\n" \
            "\t1588:	use IEEE-1588 time as master time source."


#define DOT_CMD_STR_DISCRETE \
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
            "\t12 : Port 1 Link Up\n" \
            "\t13 : Port 2 Link Up\n" \
            "\t14 : Port 3 Link Up\n" \
            "\t15 : Port 4 Link Up\n" \
            "\t16 : Port 5 Link Up\n" \
            "\t17 : Port 6 Link Up\n" \
            "\t24 : Time Lock\n" \
            "\t25 : 1588 Lock\n" \
            "\t26 : GPS Lock\n" \
            "\t27 : Switch Error\n" \
            "\t28 : FPGA Error\n" \
            "\t29 : User Control\n" \
            "\t30  : disable\n"


#define DOT_CMD_STR_DISOUT_HELP_STR \
        "Accepts a hexadecimal value and sets discrete outputs on or off if the output is configured to be under USER CONTROL.\n" \
            "The minimum value being 0 and maximum being FF.\n" \
            "See the .DISCRETE for more details on how to configure for USER CONTROL.\n"


#else 
#error "Not including help file"
#endif /* BOARD_VNS_6_REF */ 

#endif /* VNS_FPGA_CLI_6_PORT_H_ */
