/*
 * 
 *
 *  Created on: Oct 31 2022
 *      Author: Jason Alden
 *      Copyright: Telspan Data 2022
 */

#ifndef _DOT_COMMAND_STRINGS_H_
#define _DOT_COMMAND_STRINGS_H_

/***************************************************************************/
/*                          iES Dot Commands                               */
/***************************************************************************/

#define VNS_NULL    ((void *)0)
#define SQ_BKT(x)   "["x"]"

#define DOT_CMD_STR_1588             ".1588" 
#define DOT_CMD_STR_1588_ARG_MSTR_SLV "master|slave|disable" 
#define DOT_CMD_STR_1588_ARG_MD_CLK   "modify_clock" 
#define DOT_CMD_STR_1588_ARGS        SQ_BKT( DOT_CMD_STR_1588_ARG_MSTR_SLV ) " " SQ_BKT( DOT_CMD_STR_1588_ARG_MD_CLK ) 
#define DOT_CMD_STR_1588_CMD         DOT_CMD_STR_1588 " " DOT_CMD_STR_1588_ARGS
#define DOT_CMD_STR_1588_DESC        "Gets or sets the IEEE-1588 Mode for the FPGA."
#define DOT_CMD_STR_1588_FUNC        FPGA_dot_1588_cmd

#define DOT_CMD_STR_BIT              ".BIT" 
#define DOT_CMD_STR_BIT_ARGS         "" 
#define DOT_CMD_STR_BIT_DESC         "FPGA Built In Test command"
#define DOT_CMD_STR_BIT_CMD        DOT_CMD_STR_BIT
#define DOT_CMD_STR_BIT_FUNC          FPGA_dot_bit_cmd

#define DOT_CMD_STR_CONFIG           ".CONFIG" 
#define DOT_CMD_STR_CONFIG_ARGS      "" 
#define DOT_CMD_STR_CONFIG_DESC      "Show FPGA Configuration."
#define DOT_CMD_STR_CONFIG_CMD         DOT_CMD_STR_CONFIG
#define DOT_CMD_STR_CONFIG_FUNC       FPGA_dot_config_cmd

#define DOT_CMD_STR_DC1PPS           ".DC1PPS " 
#define DOT_CMD_STR_DC1PPS_ARGS      "on|off" 
#define DOT_CMD_STR_DC1PPS_DESC      "Gets or sets the IRIG-DC output type: on = 1PPS, off = IRIG-DC."
#define DOT_CMD_STR_DC1PPS_CMD         DOT_CMD_STR_DC1PPS " " SQ_BKT( DOT_CMD_STR_DC1PPS_ARGS )
#define DOT_CMD_STR_DC1PPS_FUNC       FPGA_dot_dc_1pps_cmd

#define DOT_CMD_STR_DISCRETE         ".DISCRETE"
#define DOT_CMD_STR_DISCRETE_ARGS    "[in|out] [<disc_num>] [<disc_setting>]" 
#define DOT_CMD_STR_DISCRETE_DESC    "FPGA Discrete command."
#define DOT_CMD_STR_DISCRETE_CMD     DOT_CMD_STR_DISCRETE " " DOT_CMD_STR_DISCRETE_ARGS
#define DOT_CMD_STR_DISCRETE_FUNC     FPGA_dot_discrete_cmd

#define DOT_CMD_STR_DISOUT           ".DISOUT" 
#define DOT_CMD_STR_DISOUT_ARGS      "[<value>]" 
#define DOT_CMD_STR_DISOUT_DESC      "Sets discrete outputs defined as USER"
#define DOT_CMD_STR_DISOUT_CMD         DOT_CMD_STR_DISOUT " " DOT_CMD_STR_DISOUT_ARGS
#define DOT_CMD_STR_DISOUT_FUNC       FPGA_dot_disout_cmd

#define DOT_CMD_STR_GPS              ".GPS" 
#define DOT_CMD_STR_GPS_ARGS         "0|3|5" 
#define DOT_CMD_STR_GPS_DESC         "Displays or sets the GPS Configuration."
#define DOT_CMD_STR_GPS_CMD          DOT_CMD_STR_GPS " " SQ_BKT( DOT_CMD_STR_GPS_ARGS )
#define DOT_CMD_STR_GPS_FUNC          FPGA_dot_gps_cmd

#define DOT_CMD_STR_EPE              ".EPE Port"
#define DOT_CMD_STR_EPE_ARGS         "[encode|decode|disable] [hdlc|ch7] [<bit_rate>] [<ch7_word_count>]"
#define DOT_CMD_STR_EPE_DESC         "Set or show the EPE port"
#define DOT_CMD_STR_EPE_CMD        DOT_CMD_STR_EPE " " DOT_CMD_STR_EPE_ARGS
#define DOT_CMD_STR_EPE_FUNC          cli_cmd_epe_port

#define DOT_CMD_STR_HEALTH           ".HEALTH"
#define DOT_CMD_STR_HEALTH_ARGS      "[<feature>]" 
#define DOT_CMD_STR_HEALTH_DESC      "Displays FPGA Health Information."
#define DOT_CMD_STR_HEALTH_CMD         DOT_CMD_STR_HEALTH " " DOT_CMD_STR_HEALTH_ARGS
#define DOT_CMD_STR_HEALTH_FUNC       FPGA_dot_health_cmd

#define DOT_CMD_STR_HELP             ".HELP" 
#define DOT_CMD_STR_HELP_ARGS        "" 
#define DOT_CMD_STR_HELP_DESC        "Displays all dot commands."
#define DOT_CMD_STR_HELP_CMD           DOT_CMD_STR_HELP
#define DOT_CMD_STR_HELP_FUNC         FPGA_dot_health_cmd

#define DOT_CMD_STR_INFO             ".INFO" 
#define DOT_CMD_STR_INFO_ARGS        "" 
#define DOT_CMD_STR_INFO_DESC        "Gets iES product information."
#define DOT_CMD_STR_INFO_CMD           DOT_CMD_STR_INFO
#define DOT_CMD_STR_INFO_FUNC         FPGA_dot_info_cmd

#define DOT_CMD_STR_RESET            ".RESET" 
#define DOT_CMD_STR_RESET_ARGS       "" 
#define DOT_CMD_STR_RESET_DESC       "Resets the switch."
#define DOT_CMD_STR_RESET_CMD          DOT_CMD_STR_RESET
#define DOT_CMD_STR_RESET_FUNC        FPGA_dot_reset_cmd

#define DOT_CMD_STR_SAVE             ".SAVE" 
#define DOT_CMD_STR_SAVE_ARGS        "" 
#define DOT_CMD_STR_SAVE_DESC        "Save FPGA Configuration."
#define DOT_CMD_STR_SAVE_CMD           DOT_CMD_STR_SAVE
#define DOT_CMD_STR_SAVE_FUNC         FPGA_dot_save_cmd

#define DOT_CMD_STR_STATUS           ".STATUS" 
#define DOT_CMD_STR_STATUS_ARGS      "" 
#define DOT_CMD_STR_STATUS_DESC      "FPGA Status command"
#define DOT_CMD_STR_STATUS_CMD         DOT_CMD_STR_STATUS
#define DOT_CMD_STR_STATUS_FUNC       FPGA_dot_status_cmd

#define DOT_CMD_STR_TIME             ".TIME" 
#define DOT_CMD_STR_TIME_ARGS        "" 
#define DOT_CMD_STR_TIME_DESC        "Shows the system time."
#define DOT_CMD_STR_TIME_CMD           DOT_CMD_STR_TIME
#define DOT_CMD_STR_TIME_FUNC         FPGA_dot_time_cmd

#define DOT_CMD_STR_TIMIN            ".TIMIN"
#define DOT_CMD_STR_TIMIN_ARGS       ""
#define DOT_CMD_STR_TIMIN_DESC       "Gets or sets the time in configuration."
#define DOT_CMD_STR_TIMIN_CMD          DOT_CMD_STR_TIMIN
#define DOT_CMD_STR_TIMIN_FUNC        FPGA_dot_time_in_cmd

#define DOT_CMD_STR_TIMOUT           ".TIMOUT" 
#define DOT_CMD_STR_TIMOUT_ARGS      "[none|a|b|g]" 
#define DOT_CMD_STR_TIMOUT_DESC      "Gets or sets the time out configuration."
#define DOT_CMD_STR_TIMOUT_CMD         DOT_CMD_STR_TIMOUT " " DOT_CMD_STR_TIMOUT_ARGS
#define DOT_CMD_STR_TIMOUT_FUNC       FPGA_dot_time_out_cmd

#define DOT_CMD_STR_TCAL             ".TCAL" 
#define DOT_CMD_STR_TCAL_ARGS        "[in|out] [cal1] [cal2] [cal3]" 
#define DOT_CMD_STR_TCAL_DESC        "Gets or sets the time input or output calibrations."
#define DOT_CMD_STR_TCAL_CMD           DOT_CMD_STR_TCAL " " DOT_CMD_STR_TCAL_ARGS
#define DOT_CMD_STR_TCAL_FUNC         FPGA_dot_tcal_cmd

#define DOT_CMD_STR_VERSION          ".VERSION" 
#define DOT_CMD_STR_VERSION_ARGS     "" 
#define DOT_CMD_STR_VERSION_DESC     "Gets the current VNS type and version."
#define DOT_CMD_STR_VERSION_CMD        DOT_CMD_STR_VERSION
#define DOT_CMD_STR_VERSION_FUNC      FPGA_dot_version_cmd
 
#define DOT_CMD_STR_SN               ".SN" 
#define DOT_CMD_STR_SN_ARGS          "[<integer>]" 
#define DOT_CMD_STR_SN_DESC          "Gets serial number of switch board or Sets MAC address one time when unit is being commissioned"
#define DOT_CMD_STR_SN_FUNC           FPGA_dot_sn_cmd
#define DOT_CMD_STR_SN_CMD         DOT_CMD_STR_SN " " DOT_CMD_STR_SN_ARGS
 

/***************************************************************************/
/*                          iES Debug Commands                             */
/***************************************************************************/

#define IES_DEBUG_CMD_STR_EPE_LIC          "Debug IES EPE LICENSE" 
#define IES_DEBUG_CMD_STR_EPE_LIC_ARGS     "[<sect_a>] [<sect_b>] [<sect_c>] [<sect_d>] [<sect_e>]" 
#define IES_DEBUG_CMD_STR_EPE_LIC_DESC     "Sets license for Ethernet PCM encder"
#define IES_DEBUG_CMD_STR_EPE_LIC_CMD          IES_DEBUG_CMD_STR_EPE_LIC " " IES_DEBUG_CMD_STR_EPE_LIC_ARGS
#define IES_DEBUG_CMD_STR_EPE_LIC_FUNC      cli_cmd_epe_license          

#define IES_DEBUG_CMD_STR_BRD_MDL          "Debug IES BOARD MODEL" 
#define IES_DEBUG_CMD_STR_BRD_MDL_ARGS     "ies6|ies8|ies12|1275e_ies12|ies16" 
#define IES_DEBUG_CMD_STR_BRD_MDL_DESC     "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces"
#define IES_DEBUG_CMD_STR_BRD_MDL_CMD      IES_DEBUG_CMD_STR_BRD_MDL " " SQ_BKT( IES_DEBUG_CMD_STR_BRD_MDL_ARGS )
#define IES_DEBUG_CMD_STR_BRD_MDL_FUNC      cli_cmd_ies_board_model    

#define IES_DEBUG_CMD_STR_BRD_NUM          "Debug IES BOARD NUMBER"
#define IES_DEBUG_CMD_STR_BRD_NUM_ARGS     "[power|fpga|switch|interconnect|unit_sn] [<board_id>]"
#define IES_DEBUG_CMD_STR_BRD_NUM_DESC     "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces"
#define IES_DEBUG_CMD_STR_BRD_NUM_CMD          IES_DEBUG_CMD_STR_BRD_NUM " " IES_DEBUG_CMD_STR_BRD_NUM_ARGS
#define IES_DEBUG_CMD_STR_BRD_NUM_FUNC      cli_cmd_ies_board_id
 
#define IES_DEBUG_CMD_STR_BRD_REV          "Debug IES BOARD REVISION" 
#define IES_DEBUG_CMD_STR_BRD_REV_ARGS     "[power|fpga|switch|interconnect] [<revision>]" 
#define IES_DEBUG_CMD_STR_BRD_REV_DESC     "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces"
#define IES_DEBUG_CMD_STR_BRD_REV_CMD          IES_DEBUG_CMD_STR_BRD_REV " " IES_DEBUG_CMD_STR_BRD_REV_ARGS
#define IES_DEBUG_CMD_STR_BRD_REV_FUNC      cli_cmd_board_id_revision
 
#define IES_DEBUG_CMD_STR_BRD_ID          "Debug IES BOARD ID SET" 
#define IES_DEBUG_CMD_STR_BRD_ID_ARGS     "[<hex_feature_set>]" 
#define IES_DEBUG_CMD_STR_BRD_ID_DESC     "Set by manufacturer to help software determine functionality. Do not change unless you understand the consequnces"
#define IES_DEBUG_CMD_STR_BRD_ID_CMD           IES_DEBUG_CMD_STR_BRD_ID " " IES_DEBUG_CMD_STR_BRD_ID_ARGS
#define IES_DEBUG_CMD_STR_BRD_ID_FUNC       cli_cmd_board_id
 
#define IES_DEBUG_CMD_STR_PRDT_ID          "Debug IES set product ID" 
#define IES_DEBUG_CMD_STR_PRDT_ID_ARGS     "" 
#define IES_DEBUG_CMD_STR_PRDT_ID_DESC     "Gets IES product or Sets product id in MAC address of switch :1 for iES-6, 2 for iES-12 "
#define IES_DEBUG_CMD_STR_PRDT_ID_CMD          IES_DEBUG_CMD_STR_PRDT_ID
#define IES_DEBUG_CMD_STR_PRDT_ID_FUNC      FPGA_set_product_id_cmd
 
#define IES_DEBUG_CMD_STR_I2C_READ         "Debug .READ" 
#define IES_DEBUG_CMD_STR_I2C_READ_ARGS    "<i2c_reg>" 
#define IES_DEBUG_CMD_STR_I2C_READ_DESC    "Debug FPGA I2C_Read Data Word"
#define IES_DEBUG_CMD_STR_I2C_READ_CMD         IES_DEBUG_CMD_STR_I2C_READ " " IES_DEBUG_CMD_STR_I2C_READ_ARGS
#define IES_DEBUG_CMD_STR_I2C_READ_FUNC     FPGA_i2c_debug_read
 
#define IES_DEBUG_CMD_STR_I2C_WRITE        "Debug .WRITE" 
#define IES_DEBUG_CMD_STR_I2C_WRITE_ARGS   "<i2c_reg> <i2c_data_word>" 
#define IES_DEBUG_CMD_STR_I2C_WRITE_DESC   "Debug FPGA I2C_Write Data Word"
#define IES_DEBUG_CMD_STR_I2C_WRITE_CMD        IES_DEBUG_CMD_STR_I2C_WRITE " " IES_DEBUG_CMD_STR_I2C_WRITE_ARGS
#define IES_DEBUG_CMD_STR_I2C_WRITE_FUNC    FPGA_i2c_debug_write   

#define IES_DEBUG_CMD_STR_DEBUG            "Debug .DEBUG" 
#define IES_DEBUG_CMD_STR_DEBUG_ARGS       "[on|off]" 
#define IES_DEBUG_CMD_STR_DEBUG_DESC       "Debug FPGA I2C_Write Data Word"
#define IES_DEBUG_CMD_STR_DEBUG_CMD        IES_DEBUG_CMD_STR_DEBUG " " IES_DEBUG_CMD_STR_DEBUG_ARGS
#define IES_DEBUG_CMD_STR_DEBUG_FUNC        FPGA_dot_debug   

#define IES_DEBUG_CMD_STR_TIMER            "Debug .timer" 
#define IES_DEBUG_CMD_STR_TIMER_ARGS       "[on|off]" 
#define IES_DEBUG_CMD_STR_TIMER_DESC       "Debug FPGA I2C_Write Data Word"
#define IES_DEBUG_CMD_STR_TIMER_CMD        IES_DEBUG_CMD_STR_TIMER " " IES_DEBUG_CMD_STR_TIMER_ARGS
#define IES_DEBUG_CMD_STR_TIMER_FUNC        FPGA_dot_timer   

#define IES_DEBUG_CMD_STR_CONFIG           "Debug .CONFIG" 
#define IES_DEBUG_CMD_STR_CONFIG_ARGS      "" 
#define IES_DEBUG_CMD_STR_CONFIG_DESC      "Gets the current FPGA configuration."
#define IES_DEBUG_CMD_STR_CONFIG_CMD           IES_DEBUG_CMD_STR_CONFIG
#define IES_DEBUG_CMD_STR_CONFIG_FUNC       FPGA_dot_debug_config_cmd   

#define IES_DEBUG_CMD_STR_REGDUMP          "Debug .REGDUMP" 
#define IES_DEBUG_CMD_STR_REGDUMP_ARGS     "" 
#define IES_DEBUG_CMD_STR_REGDUMP_DESC     "Gets the current FPGA configuration."
#define IES_DEBUG_CMD_STR_REGDUMP_CMD          IES_DEBUG_CMD_STR_REGDUMP
#define IES_DEBUG_CMD_STR_REGDUMP_FUNC      FPGA_dot_debug_regdump_cmd   

#define IES_DEBUG_CMD_STR_EPE_PORT         "Debug IES EPE PORT" 
#define IES_DEBUG_CMD_STR_EPE_PORT_ARGS    "[hdlc|ch7] [<bit_rate>] [<ch7_word_count>]" 
#define IES_DEBUG_CMD_STR_EPE_PORT_DESC    "Set or show the EPE port"
#define IES_DEBUG_CMD_STR_EPE_PORT_CMD         IES_DEBUG_CMD_STR_EPE_PORT " " IES_DEBUG_CMD_STR_EPE_PORT_ARGS
#define IES_DEBUG_CMD_STR_EPE_PORT_FUNC     cli_cmd_epe_debug   
 
/***************************************************************************/
/*                          iES Debug Commands                             */
/***************************************************************************/

#define CLI_COMMANDS_BRACKET(x,y) { x, y },


#define CLI_CMD_TAB_ENTRY_MACRO(w,x,y,z) \
    cli_cmd_tab_entry ( \
            w, \
            x, \
            y, \
            CLI_CMD_SORT_KEY_DEFAULT, \
            CLI_CMD_TYPE_CONF, \
            VTSS_MODULE_ID_VNS_FPGA, \
            z, \
            vns_fpga_cli_req_default_set, \
            vns_fpga_cli_parm_table, \
            CLI_CMD_FLAG_NONE \
     );


#endif /* _DOT_COMMAND_STRINGS_H_ */ 

#if 0 /* COMMENT_OUT */ 
 
/*Add function  Macro
0f"yi"/"GNjjjjjyiwLggnjjjpjj0
*/

/***************************************************************************/
/*                          iES Dot CLI HELP Commands                      */
/***************************************************************************/

#define VNS_FPGA_HELP_STRING     \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_1588,        DOT_CMD_STR_1588_DESC    ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_BIT,         DOT_CMD_STR_BIT_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_CONFIG,      DOT_CMD_STR_CONFIG_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DC1PPS,      DOT_CMD_STR_DC1PPS_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISCRETE,    DOT_CMD_STR_DISCRETE_DESC) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_DISOUT,      DOT_CMD_STR_DISOUT_DESC  ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_GPS,         DOT_CMD_STR_GPS_DESC     ) \
    CLI_COMMANDS_BRACKET( DOT_CMD_STR_EPE,         DOT_CMD_STR_EPE_DESC     ) \
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

Macro to create structure below. First yank 'CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),'
Pa  ea, Jea, Jea, Jea )jj0

#define ADD_CLI_TAB_ENTRIES() \
        /***************************************************************************/ \
        /*                          iES DOT Commands                               */ \
        /***************************************************************************/ \

        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_1588,      DOT_CMD_STR_1588_ARGS,        DOT_CMD_STR_1588_DESC,        DOT_CMD_STR_1588_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_BIT,       DOT_CMD_STR_BIT_ARGS,         DOT_CMD_STR_BIT_DESC,         DOT_CMD_STR_BIT_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_CONFIG,    DOT_CMD_STR_CONFIG_ARGS,      DOT_CMD_STR_CONFIG_DESC,      DOT_CMD_STR_CONFIG_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DC1PPS,    DOT_CMD_STR_DC1PPS_ARGS,      DOT_CMD_STR_DC1PPS_DESC,      DOT_CMD_STR_DC1PPS_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DISCRETE,  DOT_CMD_STR_DISCRETE_ARGS,    DOT_CMD_STR_DISCRETE_DESC,    DOT_CMD_STR_DISCRETE_FUNC) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_DISOUT,    DOT_CMD_STR_DISOUT_ARGS,      DOT_CMD_STR_DISOUT_DESC,      DOT_CMD_STR_DISOUT_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_GPS,       DOT_CMD_STR_GPS_ARGS,         DOT_CMD_STR_GPS_DESC,         DOT_CMD_STR_GPS_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_EPE,       DOT_CMD_STR_EPE_ARGS,         DOT_CMD_STR_EPE_DESC,         DOT_CMD_STR_EPE_FUNC     ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_HEALTH,    DOT_CMD_STR_HEALTH_ARGS,      DOT_CMD_STR_HEALTH_DESC,      DOT_CMD_STR_HEALTH_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_HELP,      DOT_CMD_STR_HELP_ARGS,        DOT_CMD_STR_HELP_DESC,        DOT_CMD_STR_HELP_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_INFO,      DOT_CMD_STR_INFO_ARGS,        DOT_CMD_STR_INFO_DESC,        DOT_CMD_STR_INFO_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_RESET,     DOT_CMD_STR_RESET_ARGS,       DOT_CMD_STR_RESET_DESC,       DOT_CMD_STR_RESET_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_SAVE,      DOT_CMD_STR_SAVE_ARGS,        DOT_CMD_STR_SAVE_DESC,        DOT_CMD_STR_SAVE_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_STATUS,    DOT_CMD_STR_STATUS_ARGS,      DOT_CMD_STR_STATUS_DESC,      DOT_CMD_STR_STATUS_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIME,      DOT_CMD_STR_TIME_ARGS,        DOT_CMD_STR_TIME_DESC,        DOT_CMD_STR_TIME_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIMIN,     DOT_CMD_STR_TIMIN_ARGS,       DOT_CMD_STR_TIMIN_DESC,       DOT_CMD_STR_TIMIN_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TIMOUT,    DOT_CMD_STR_TIMOUT_ARGS,      DOT_CMD_STR_TIMOUT_DESC,      DOT_CMD_STR_TIMOUT_FUNC  ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_TCAL,      DOT_CMD_STR_TCAL_ARGS,        DOT_CMD_STR_TCAL_DESC,        DOT_CMD_STR_TCAL_FUNC    ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_VERSION,   DOT_CMD_STR_VERSION_ARGS,     DOT_CMD_STR_VERSION_DESC,     DOT_CMD_STR_VERSION_FUNC ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  DOT_CMD_STR_SN,        DOT_CMD_STR_SN_ARGS,          DOT_CMD_STR_SN_DESC,          DOT_CMD_STR_SN_FUNC      ) \

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
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_REGDUMP,      IES_DEBUG_CMD_STR_REGDUMP_ARGS,     IES_DEBUG_CMD_STR_REGDUMP_DESC,     IES_DEBUG_CMD_STR_REGDUMP_FUNC   ) \
        CLI_CMD_TAB_ENTRY_MACRO( ((void *)0),  IES_DEBUG_CMD_STR_EPE_PORT,     IES_DEBUG_CMD_STR_EPE_PORT_ARGS,    IES_DEBUG_CMD_STR_EPE_PORT_DESC,    IES_DEBUG_CMD_STR_EPE_PORT_FUNC  )



Adding a blank line endif needs it
#endif /* COMMENT_OUT */ 
