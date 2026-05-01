/*
 * 
 *
 *  Created on: Oct 31, 2022
 *      Author: Jason Alden
 *      Copyright: Telspan Data 2022
 */

#ifndef VNS_FPGA_CLI_IGU_PORT_H_
#define VNS_FPGA_CLI_IGU_PORT_H_

#if defined (BOARD_IGU_REF)

#include "dot_command_strings.h" 

#undef DOT_CMD_STR_HELP_FUNC
#define DOT_CMD_STR_HELP_FUNC FPGA_dot_ies_8_help_cmd




#undef DOT_CMD_STR_TIMIN_ARGS
#define DOT_CMD_STR_TIMIN_ARGS      "[none|1588] [modify_clock]"

#undef DOT_CMD_STR_HELP_FUNC
#define DOT_CMD_STR_HELP_FUNC FPGA_dot_ies_12_help_cmd


#define VNS_FPGA_HELP_STRING     \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_1588,        DOT_CMD_STR_1588_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_BIT,         DOT_CMD_STR_BIT_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_CONFIG,      DOT_CMD_STR_CONFIG_DESC  ) \
\
\
\
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
\
\
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_VERSION,     DOT_CMD_STR_VERSION_DESC ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_SN,          DOT_CMD_STR_SN_DESC      )

#define ADD_CLI_TAB_ENTRIES() \
        /***************************************************************************/ \
        /*                          iES DOT Commands                               */ \
        /***************************************************************************/ \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_1588,      DOT_CMD_STR_1588_ARGS,        DOT_CMD_STR_1588_DESC,        DOT_CMD_STR_1588_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_BIT,       DOT_CMD_STR_BIT_ARGS,         DOT_CMD_STR_BIT_DESC,         DOT_CMD_STR_BIT_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_CONFIG,    DOT_CMD_STR_CONFIG_ARGS,      DOT_CMD_STR_CONFIG_DESC,      DOT_CMD_STR_CONFIG_FUNC  ) \
\
\
\
\
\
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_HEALTH,    DOT_CMD_STR_HEALTH_ARGS,      DOT_CMD_STR_HEALTH_DESC,      DOT_CMD_STR_HEALTH_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_HELP,      DOT_CMD_STR_HELP_ARGS,        DOT_CMD_STR_HELP_DESC,        DOT_CMD_STR_HELP_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_INFO,      DOT_CMD_STR_INFO_ARGS,        DOT_CMD_STR_INFO_DESC,        DOT_CMD_STR_INFO_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_RESET,     DOT_CMD_STR_RESET_ARGS,       DOT_CMD_STR_RESET_DESC,       DOT_CMD_STR_RESET_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_SAVE,      DOT_CMD_STR_SAVE_ARGS,        DOT_CMD_STR_SAVE_DESC,        DOT_CMD_STR_SAVE_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_STATUS,    DOT_CMD_STR_STATUS_ARGS,      DOT_CMD_STR_STATUS_DESC,      DOT_CMD_STR_STATUS_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIME,      DOT_CMD_STR_TIME_ARGS,        DOT_CMD_STR_TIME_DESC,        DOT_CMD_STR_TIME_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIMIN,     DOT_CMD_STR_TIMIN_ARGS,       DOT_CMD_STR_TIMIN_DESC,       DOT_CMD_STR_TIMIN_FUNC   ) \
\
\
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_VERSION,   DOT_CMD_STR_VERSION_ARGS,     DOT_CMD_STR_VERSION_DESC,     DOT_CMD_STR_VERSION_FUNC ) \
\
        /***************************************************************************/ \
        /*                          iES Debug Commands                             */ \
        /***************************************************************************/ \
\
\
\
\
\
\
\
\
\
\
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_CONFIG,       IES_DEBUG_CMD_STR_CONFIG_ARGS,      IES_DEBUG_CMD_STR_CONFIG_DESC,      IES_DEBUG_CMD_STR_CONFIG_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_REGDUMP,      IES_DEBUG_CMD_STR_REGDUMP_ARGS,     IES_DEBUG_CMD_STR_REGDUMP_DESC,     IES_DEBUG_CMD_STR_REGDUMP_FUNC   )


#endif /* BOARD_IGU_REF */

#endif /* VNS_FPGA_CLI_IGU_PORT_H_ */
