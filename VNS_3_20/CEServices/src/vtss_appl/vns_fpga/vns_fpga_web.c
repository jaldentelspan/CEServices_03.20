/*

   Vitesse Switch API software.

   Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
   Rights Reserved.

   Unpublished rights reserved under the copyright laws of the United States of
   America, other countries and international treaties. Permission to use, copy,
   store and modify, the software and its source code is granted. Permission to
   integrate into other products, disclose, transmit and distribute the software
   in an absolute machine readable format (e.g. HEX file) is also granted.  The
   source code of the software may not be disclosed, transmitted or distributed
   without the written permission of Vitesse. The software and its source code
   may only be used in products utilizing the Vitesse switch products.

   This copyright notice must appear in any copy, modification, disclosure,
   transmission or distribution of the software. Vitesse retains all ownership,
   copyright, trade secret and proprietary rights in the software.

   THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
   INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
   FOR A PARTICULAR USE AND NON-INFRINGEMENT.
   */

#include "web_api.h"

#include "flash_mgmt_api.h"
#include "firmware_api.h"
#include "vns_fpga_api.h"
#include "vns_fpga_cli.h"   /* Check that public function decls. and defs. are in sync 	*/
#include "vns_types.h"
#include "conf_api.h"  /* Need current MAC address */

/* ==============
 * EPE/Mirror API
 */
#ifdef IES_EPE_FEATURE_ENABLE
#include "mirror_api.h"
#include "port_api.h"
#include "topo_api.h"
#include "msg_api.h"
#endif /* IES_EPE_FEATURE_ENABLE */
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_VNS_FPGA
#include <vtss_trace_api.h>
/* ============== */


#define JASONS_TESTING 1
#define CONT_WORK 1


static void send_string(CYG_HTTPD_STATE* p, const char *messege);
int getDiscreteFunct(int do_funct_value );
int getDiscreteFunctPort(int do_funct_value );
char * getDiscreteOutputIconID(int do_funct_value, int status);
char * getDiscreteInputIconID(int di_funct_value, int status);
vns_do_setting_t convertDiscreteOutputFormToEnum( int do_funct_value, int do_port_value);

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
static cyg_int32 handler_fpga_firmware_status(CYG_HTTPD_STATE* p)
{
    const char *fpga_firmware_status = fpga_firmware_status_get();
    cyg_httpd_start_chunked("html");
    cyg_httpd_write_chunked(fpga_firmware_status, strlen(fpga_firmware_status));
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_dot_fpga_discrete_trigger(CYG_HTTPD_STATE* p)
{
    u32 discrete_hex_val = 0;
    const char * jsObject = "shift_value";
    cyg_httpd_form_varable_int(p, jsObject, &discrete_hex_val);
    T_D("\ndiscrete_hex_val: %0d\n",discrete_hex_val);


    u32 do_state = 0;
    u32 bit_set = 0;

    //*** Get Discrete Output State
    if(Get_DiscreteOutputState(&do_state))//int Get_DiscreteOutputState(u32 * do_state)
    {
        T_E("Could not get Discrete Output state");
        return -1;
    }
    //*** Create Mask **
    if((discrete_hex_val > 0) &&
            (discrete_hex_val <= DISCRETE_OUT_MAX))
    {
        bit_set = 1 << (discrete_hex_val-1);
        do_state ^= bit_set;


        if(Set_DiscreteOutputState(do_state))//int Get_DiscreteOutputState(u32 * do_state)
        {
            T_E("Could not set Discrete Output state");
            return -1;
        }

    }
    else
    {
        T_E("Discrete output being set is out of range...");
        return -1;
    }
    return 0;
}

static cyg_int32 handler_dot_fpga_discrete_config(CYG_HTTPD_STATE* p)
{
    cyg_int32 retval = 0;
    char dataStr1[1000];
    char dataStr2[100];
    int i, discrete_port_cnt;
    uint32_t di_word = 0;
    discrete_in_config di_setting;
    uint32_t do_word = 0;
    discrete_out_config do_setting;

    // for CYG_HTTPD_METHOD_POST
    int form_value_funct,form_value_port ;
    vns_do_setting_t do_fortoem;
    char function_select[10];
    char port_select[10];

    if(p->method == CYG_HTTPD_METHOD_POST)
    {
        if(Get_DiscreteOutputConfig(&do_setting))
        {
            T_D("Set_DiscreteOutputConfig Failed");

        }
        else
        {

            for(discrete_port_cnt = 0; discrete_port_cnt < DISCRETE_OUT_MAX; discrete_port_cnt++)
            {

                //discrete_port_cnt = 0;
                sprintf(function_select,"funct_%d",discrete_port_cnt+1);
                cyg_httpd_form_varable_int(p, function_select, &form_value_funct);
                T_D("%s:%d",function_select,form_value_funct);

                sprintf(port_select,"port_%d",discrete_port_cnt+1);
                cyg_httpd_form_varable_int(p, port_select, &form_value_port);
                T_D("%s:%d",port_select,form_value_port);



                do_fortoem = convertDiscreteOutputFormToEnum(form_value_funct, form_value_port);

                T_D("%s", vns_do_setting_name[do_fortoem]);
                //do_fortoem = DO_SETTING_DISABLED;
                do_setting.do_setting[discrete_port_cnt] = do_fortoem;//DO_SETTING_DISABLED;
                //config_shaddow.do_config.do_setting[discrete_port_cnt] = do_setting.do_setting[discrete_port_cnt];

            }




        }
        if(Set_DiscreteOutputConfig(do_setting))
        {
            T_D("Set_DiscreteOutputConfig Failed");
        }
        else
        {
            save_vns_config();
        }
#if defined(BOARD_VNS_12_REF)
        redirect(p, "/dot_discrete_config.htm");
#endif /* BOARD_VNS_12_REF */
#if defined(BOARD_VNS_6_REF)
        redirect(p, "/dot_discrete_config_6.htm");
#endif /* BOARD_VNS_6_REF */
    }
    else
    {
        //******* Start printing outputs********//
        snprintf(dataStr1, sizeof(dataStr1),"%d",DISCRETE_OUT_MAX);

        for(discrete_port_cnt = 0; discrete_port_cnt < DISCRETE_OUT_MAX; discrete_port_cnt++)
        {
            strcat( dataStr1, "|");
            // ******* append VNS port number
            snprintf(dataStr2, sizeof(dataStr2),"%d/",VNS_PORT_COUNT);
            strcat( dataStr1, dataStr2);

            for(i = 1; i <= VNS_PORT_COUNT; i++)
            {
                // ******* append list of port names
                snprintf(dataStr2, sizeof(dataStr2),"%d/",i);
                strcat( dataStr1, dataStr2);
            }
            // ******* append specific disc
            snprintf(dataStr2, sizeof(dataStr2),"%d/",(discrete_port_cnt + 1));
            strcat( dataStr1, dataStr2);


            // ******* append port function
            if(Get_DiscreteOutputConfig(&do_setting))
            {
                T_D("Get_DiscreteOutputConfig Failed");

            }
            else
            {
                // need to parse return a port number (0 being NA) & return a function
                //snprintf(dataStr2, sizeof(dataStr2),"%s/",vns_do_setting_name[do_setting.do_setting[discrete_port_cnt]]);
                //strcat( dataStr1, dataStr2);
                //do_setting.do_setting[discrete_port_cnt]
                snprintf(dataStr2, sizeof(dataStr2),"%d/%d/",
                        getDiscreteFunct(do_setting.do_setting[discrete_port_cnt]),getDiscreteFunctPort(do_setting.do_setting[discrete_port_cnt]) );
                strcat( dataStr1, dataStr2);
                // ******* append port number, if NA maybe send 0 to disable
                // ******* append status: disabled, on, or off

            }
            if(Get_DiscreteOutputState(&do_word))
            {
                T_D("Get_DiscreteOutputState Error");
                strcat( dataStr1, "-1/");
            }
            else
            {
                snprintf(dataStr2, sizeof(dataStr2),"%s/",getDiscreteOutputIconID(
                            do_setting.do_setting[discrete_port_cnt],( ( (do_word >> discrete_port_cnt) & 0x01) ? 0 : 1)));
                strcat( dataStr1, dataStr2);
            }

        }


        snprintf(dataStr2, sizeof(dataStr2),"|%d",DISCRETE_IN_MAX);
        strcat( dataStr1, dataStr2);
        for(discrete_port_cnt = 0; discrete_port_cnt < DISCRETE_IN_MAX; discrete_port_cnt++)
        {
            if(Get_DiscreteInputConfig(&di_setting))
            {
                T_D("Get_DiscreteInputState Error");
                snprintf(dataStr2, sizeof(dataStr2), "|%d/-1/-1/",discrete_port_cnt);
                strcat( dataStr1, dataStr2);
            }
            else
            {
                snprintf(dataStr2, sizeof(dataStr2),"|%d/%d/%s/",
                        discrete_port_cnt+1,( (di_word >> i) & 0x01), getDiscreteInputIconID(
                            di_setting.di_setting[discrete_port_cnt], (((di_word >> i) & 0x01) ? 1 : 0)) );
                strcat( dataStr1, dataStr2);
            }
        }



        strcat( dataStr1, "|");
        send_string(p, dataStr1);
    }
    return retval;
}

vns_do_setting_t convertDiscreteOutputFormToEnum( int do_funct_value, int do_port_value)
{
    switch(do_funct_value)
    {
        case 0:

            return (do_port_value - 1);
        case 1:

            return (do_port_value + 11);
        case 2:

            return DO_SETTING_TIME_LOCK;
        case 3:

            return DO_SETTING_1588_LOCK;
        case 4:

            return DO_SETTING_GPS_LOCK;
        case 5:

            return DO_SETTING_SWITCH_ERROR;
        case 6:

            return DO_SETTING_FPGA_ERROR;
        case 7:

            return DO_SETTING_USER;
        case 8:

            return DO_SETTING_DISABLED;
        default:
            T_D("Option for conversion did not exist:%d",do_funct_value);

            return DO_SETTING_DISABLED;
    }

}
char * getDiscreteOutputIconID(int do_funct_value, int status)
{
    if(do_funct_value == DO_SETTING_DISABLED)
    {
        return "off-small";
    }
    else if(status)
    {
        return "up";
    }
    return "down";

}
char * getDiscreteInputIconID(int di_funct_value, int status)
{
    T_D("di_funct: %d Status: %d",di_funct_value, status);

    if(di_funct_value == DI_SETTING_DISABLED)
    {
        return "off-small";
    }
    else if(status)
    {
        return "up";
    }
    return "down";
}
int getDiscreteFunct(int do_funct_value )
{
    //	int discrete_port_cnt;
    if(do_funct_value < 0 &&
            do_funct_value > 29)
    {
        return -1; // catch values out of bound
    }
    else if(do_funct_value >= 0 &&
            do_funct_value <= 11)
    {
        return 0;
    }
    else if(do_funct_value >= 12 &&
            do_funct_value <= 23)
    {
        return 1;
    }
    else if(do_funct_value == DO_SETTING_TIME_LOCK)
    {
        return 2;
    }
    else if(do_funct_value == DO_SETTING_1588_LOCK)
    {
        return 3;
    }
    else if(do_funct_value == DO_SETTING_GPS_LOCK)
    {
        return 4;
    }
    else if(do_funct_value == DO_SETTING_SWITCH_ERROR)
    {
        return 5;
    }
    else if(do_funct_value == DO_SETTING_FPGA_ERROR)
    {
        return 6;
    }
    else if(do_funct_value == DO_SETTING_USER)
    {
        return 7;
    }
    else if(do_funct_value == DO_SETTING_DISABLED)
    {
        return 8;
    }
    return -1;
}

int getDiscreteFunctPort(int do_funct_value )
{
    //	int discrete_port_cnt;
    if(do_funct_value < 0 ||
            do_funct_value > 23)
    {
        return -1; // catch values out of bound
    }
    else if(do_funct_value >= 0 &&
            do_funct_value <= 11)
    {
        return (do_funct_value +1);
    }
    else if(do_funct_value >= 12 &&
            do_funct_value <= 23)
    {
        return (do_funct_value-11);
    }
    return -1;
}

static cyg_int32 handler_fpga_upload_invalid(CYG_HTTPD_STATE* p)
{
    // **** To create string to pass to client
    char str1[100];

    snprintf(str1, sizeof(str1),get_fpga_update_invalid_string());
    send_string(p, str1);

    return -1;
}
static cyg_int32 handler_dot_fpga_status(CYG_HTTPD_STATE* p)
{
    // **** To create string to pass to client
    char str1[1000];
    char str2[100];
    //		char str3[12];
    //		int  len ;
    u16 is_facory_img;
    uint32_t temp;
    // **** For Getting time
    int year, yday, hour,minute, second;
    vns_time_input_src_t time_in_src;
    // **** For retrieving status
    vns_status_t status;
    int percent_complete;
    int warning_bits = 0; // not sure
    int error_bits = 0; // count number of health bits set (all systems, I think)
    int i;
    u8 fw_version_major = 0, fw_version_minor = 0;


    //*********************	FPGA Mode	****************************//
    /* int Get_IsUpdateFactoryImg(u16 * val);
     * need to alert user if the fpga is not in application mode, update must have failed
     *
     *
     */
    if(Get_IsUpdateFactoryImg(&is_facory_img) == 0) {
        if(is_facory_img == 1) {
            snprintf(str1, sizeof(str1),"FACTORY,");
        }
        else {
            snprintf(str1, sizeof(str1),"APPLICATION,");
        }
    }
    else {
        snprintf(str1, sizeof(str1),"ERROR,");
    }
    T_D("Is Factory img? %u", is_facory_img);
    //strcat( str1, str2);
    //*********************	MAC address check	****************************//

    if(is_mac_default()) {
        snprintf(str2, sizeof(str2),"MAC_DEFAULT,");
    }
    else {
        snprintf(str2, sizeof(str2),"MAC_SET,");
    }
    strcat( str1, str2);
    //*********************	VERSION cmd	****************************//

    snprintf(str2, sizeof(str2),".VERSION,");
    strcat( str1, str2);
    snprintf(str2, sizeof(str2), "%s: v%d.%02d,",MODEL_NAME, VNS_VERSION_MAJOR, VNS_VERSION_MINOR);
    strcat( str1, str2);

    if(0 == Get_FPGAFirmwareVersionMinor(&fw_version_minor) && 0 == Get_FPGAFirmwareVersionMajor(&fw_version_major) ) {
        snprintf(str2, sizeof(str2),"%s: v%d.%02d,","FPGA", fw_version_major, fw_version_minor);
        strcat( str1, str2);
    }
    else {
        snprintf(str2, sizeof(str2),"Error: retrieving FPGA version,");
        strcat( str1, str2);
    }
#if !defined( BOARD_VNS_8_REF )
    //*********************	SYSTEM Time	****************************//
    snprintf(str2, sizeof(str1),".TIME,");
    strcat( str1, str2);
    if(0 == Get_TimeInSource(&time_in_src))
    {
        switch(time_in_src)
        {
            case TIME_INPUT_SRC_1588:
                if(0 == Get_PTP_SetTime(&year, &yday, &hour, &minute, &second))	{
                    snprintf(str2, sizeof(str2),"Time In (%s):  %03d-%02d:%02d:%02d,",
                            TIME_IN_SRC_STR[time_in_src], yday, hour, minute, second);
                    strcat( str1, str2);
                }
                break;
            default:
                if(0 == Get_TimeIn_Time(&year, &yday, &hour, &minute, &second))	{
                    snprintf(str2, sizeof(str2),"Time In (%s):  %03d-%02d:%02d:%02d,",
                            TIME_IN_SRC_STR[time_in_src], yday, hour, minute, second);
                    strcat( str1, str2);
                }
                break;
        }

        if(0 == Get_TimeOut_Time(&year, &yday, &hour, &minute, &second))
        {
            snprintf(str2, sizeof(str2),"Time Out: %03d-%02d:%02d:%02d,", yday, hour, minute, second);
            strcat( str1, str2);
        }
    }
    else
    {
        snprintf(str2, sizeof(str2),"Time In: Error,Time Out: Error,");//, yday, hour, minute, second);
        strcat( str1, str2);
    }
#endif /* BOARD_VNS_8_REF */

    //*********************	STATUS cmd	****************************//

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
    snprintf(str2, sizeof(str2),".STATUS,");
    strcat( str1, str2);
    snprintf(str2, sizeof(str2),"S %02d %d %d,", (int)status, warning_bits, error_bits);
    strcat( str1, str2);
    switch(status)
    {
        case VNS_STATE_BIT:
            if(GetPercentComplete(&percent_complete) == 0)
            {
                snprintf(str2, sizeof(str2)," %d%%,", percent_complete);
                strcat( str1, str2);
            }
            break;
        case VNS_STATE_BUSY:
            if(GetPercentComplete(&percent_complete) == 0)
            {
                snprintf(str2, sizeof(str2)," %d%%,", percent_complete);
                strcat( str1, str2);
            }
            break;
        default:
            break;
    }
    //		if(verbose)
    if((status >= VNS_STATE_FAIL) && (status < VNS_STATE_UNKNOWN))
        snprintf(str2, sizeof(str2)," %s,", vns_status_name[status]);
    /*			else
                        CPRINTF("\n");
                        else
                        CPRINTF("\n");
                        */
#if !defined( BOARD_VNS_8_REF )
    //*********************	SYSTEM health	****************************//
    GetSystemHealthReg(&temp);
    snprintf(str2, sizeof(str2),".SYSTEM Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }
    if(temp & SYS_HEALTH_OPERATION_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Operation Failure,", SYS_HEALTH_OPERATION_FAIL);
        strcat( str1, str2);
    }
    if(temp & SYS_HEALTH_BUSY)
    {
        snprintf(str2, sizeof(str2),"%08X Busy,", SYS_HEALTH_BUSY);
        strcat( str1, str2);
    }
    if(temp & SYS_HEALTH_POWER_BOARD_OVER_TEMP)
    {
        snprintf(str2, sizeof(str2),"%08X Power Board Over-temp,", SYS_HEALTH_POWER_BOARD_OVER_TEMP);
        strcat( str1, str2);
    }
    if(temp & SYS_HEALTH_FPGA_BOARD_PSC_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Power System Controller Failure,", SYS_HEALTH_FPGA_BOARD_PSC_FAIL);
        strcat( str1, str2);
    }
    if(temp & SYS_HEALTH_GPS_FAULT)
    {
        snprintf(str2, sizeof(str2),"%08X GPS Fault,", SYS_HEALTH_GPS_FAULT);
        strcat( str1, str2);
    }
#if !defined(BOARD_IGU_REF)
    //*********************	DISCRETE IN health	****************************//
    GetDiscreteInHealthReg(&temp);
    snprintf(str2, sizeof(str2),".DISCRETE IN Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }

    //*********************	DISCRETE OUT	****************************//
    GetDiscreteOutHealthReg(&temp);
    snprintf(str2, sizeof(str2),".DISCRETE OUT Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }
#endif
#ifdef BOARD_VNS_12_REF
    //*********************	GPS	****************************//
    GetGPSHealthReg(&temp);
    snprintf(str2, sizeof(str2),".GPS Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }
    if(temp & GPS_HEALTH_NO_ANTENNA_SIGNAL)
    {
        snprintf(str2, sizeof(str2),"%08X No Signal,", GPS_HEALTH_NO_ANTENNA_SIGNAL);
        strcat( str1, str2);
    }
    if(temp & GPS_HEALTH_BAD_ANTENNA_SIGNAL)
    {
        snprintf(str2, sizeof(str2),"%08X Bad Signal,", GPS_HEALTH_BAD_ANTENNA_SIGNAL);
        strcat( str1, str2);
    }
    if(temp & GPS_HEALTH_TIME_SYNC_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Time Sync Failure,", GPS_HEALTH_TIME_SYNC_FAIL);
        strcat( str1, str2);
    }
    if(temp & GPS_HEALTH_NO_SATELLITES)
    {
        snprintf(str2, sizeof(str2),"%08X No Satellites,", GPS_HEALTH_NO_SATELLITES);
        strcat( str1, str2);
    }
#endif /* BOARD_VNS_12_REF */
    //*********************	TIME IN	****************************//

    GetTimeInHealth(&temp);
    snprintf(str2, sizeof(str2),".TIME IN Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }
    if(temp & TI_HEALTH_NO_SIGNAL)
    {
        snprintf(str2, sizeof(str2),"%08X No Signal,", TI_HEALTH_NO_SIGNAL);
        strcat( str1, str2);
    }
    if(temp & TI_HEALTH_BAD_SIGNAL)
    {
        snprintf(str2, sizeof(str2),"%08X Bad Signal,", TI_HEALTH_BAD_SIGNAL);
        strcat( str1, str2);
    }
    if(temp & TI_HEALTH_SYNC_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Sync Failure,", TI_HEALTH_SYNC_FAIL);
        strcat( str1, str2);
    }

    //*********************	TIME OUT	****************************//
#if !defined(BOARD_IGU_REF)

    GetTimeOutHealthReg(&temp);
    snprintf(str2, sizeof(str2),".TIME OUT Error(s),");
    strcat( str1, str2);
    if(temp & HEALTH_BIT_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
        strcat( str1, str2);
    }
    if(temp & HEALTH_SETUP_FAIL)
    {
        snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
        strcat( str1, str2);
    }
    if(temp & TO_HEALTH_NO_SOURCE)
    {
        snprintf(str2, sizeof(str2),"%08X No Source,", TO_HEALTH_NO_SOURCE);
        strcat( str1, str2);
    }
    if(temp & TO_HEALTH_TCG_FREEWHEEL)
    {
        snprintf(str2, sizeof(str2),"%08X Freewheel,", TO_HEALTH_TCG_FREEWHEEL);
        strcat( str1, str2);
    }

#endif
#endif /* BOARD_VNS_8_REF */
    /* concatenates str1 and str2 */

    send_string(p, str1);


    return -1;
}


static cyg_int32 handler_dot_timout_config(CYG_HTTPD_STATE* p)
{

    int rc;
    vns_time_output time_config;
    size_t len;
    const char *form_value;
    char unescaped_form_value[32];

    if(p->method == CYG_HTTPD_METHOD_POST)
    {

        form_value = cyg_httpd_form_varable_string(p, "dotTIMOUTMode", &len);
        rc = cgi_unescape(form_value, unescaped_form_value, len, sizeof(unescaped_form_value));
        form_value = unescaped_form_value;

        if(strcmp(form_value, "TIME_TYPE_IRIG_B") == 0)
        {
            time_config.timecode = TIME_OUTPUT_TYPE_IRIG_B;

        }
        else if (strcmp(form_value, "TIME_TYPE_IRIG_A") == 0)
        {
            time_config.timecode = 	TIME_OUTPUT_TYPE_IRIG_A;
        }
        else if (strcmp(form_value, "TIME_TYPE_IRIG_G") == 0)
        {
            time_config.timecode = TIME_OUTPUT_TYPE_IRIG_G;
        }
        else if (strcmp(form_value, "TIME_TYPE_IRIG_DISABLED") == 0)
        {
            time_config.timecode = TIME_OUTPUT_TYPE_DISABLED;
        }
        else
        {
            T_D("Could not find a matching form value");

        }
        T_D(form_value);

        if(Set_TimeOutTimeCode(time_config.timecode))
        {
            T_D("Set_TimeOutTimeCode Failed");
        }
        else
        {
            save_vns_config();
        }
        redirect(p, "/dot_timout_config.htm");
    }
    else
    {
        if(!Get_TimeOutConfig(&time_config))
        {
            switch(time_config.timecode)
            {
                case TIME_OUTPUT_TYPE_IRIG_B:
                    send_string( p, "TIME_TYPE_IRIG_B");
                    break;
                case TIME_OUTPUT_TYPE_IRIG_A:
                    send_string( p, "TIME_TYPE_IRIG_A");
                    break;
                case TIME_OUTPUT_TYPE_IRIG_G:
                    send_string( p, "TIME_TYPE_IRIG_G");
                    break;
                case TIME_OUTPUT_TYPE_DISABLED:
                    send_string( p, "TIME_TYPE_IRIG_DISABLED");
                    break;
                default:
                    send_string( p, "Error");
                    break;
            }
        }
        else
        {
            T_D( "Get_TimeOutConfig Failed!");
        }
    }
    return -1;
}

static cyg_int32 handler_dot_timin_config(CYG_HTTPD_STATE* p)
{
    int rc, i;
    vns_time_input time_config;
    vns_1588_type mode;
    //int form_value;
    size_t len;
    const char *form_value;
    char unescaped_form_value[32];
    char str1[1000]; memset( str1, 0, 1000);
    char str2[100]; memset( str2, 0, 100);
    u8 fw_version_major, fw_version_minor;
    
    if(Get_TimeInConfig(&time_config))
    {
        T_D("Get_TimeInConfig Failed");
    }
    else
    {

        if(p->method == CYG_HTTPD_METHOD_POST)
        {
            //cyg_httpd_form_varable_int(p, "dotTIMINMode", &form_value);
            form_value = cyg_httpd_form_varable_string(p, "timinselect", &len);
            rc = cgi_unescape(form_value, unescaped_form_value, len, sizeof(unescaped_form_value));
            form_value = unescaped_form_value;

            if(strcmp(form_value, TIME_IN_SRC_ENUM_STR[TIME_INPUT_SRC_IRIG_A] ) == 0)
            {
                time_config.source = TIME_INPUT_SRC_IRIG_A;
                time_config.signal = TIME_INPUT_TYPE_DC;
                if(!Get_IEEE_1588_Mode(&mode)) {
                    if(mode == IEEE_1588_SLAVE) {
                        Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
                    }
                }
            }
            else if(strcmp(form_value, TIME_IN_SRC_ENUM_STR[TIME_INPUT_SRC_IRIG_B]) == 0)
            {
                time_config.source = TIME_INPUT_SRC_IRIG_B;
                time_config.signal = TIME_INPUT_TYPE_DC;
                if(!Get_IEEE_1588_Mode(&mode)) {
                    if(mode == IEEE_1588_SLAVE) {
                        Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
                    }
                }
            }
            else if(strcmp(form_value, TIME_IN_SRC_ENUM_STR[TIME_INPUT_SRC_IRIG_G]) == 0)
            {
                time_config.source = TIME_INPUT_SRC_IRIG_G;
                time_config.signal = TIME_INPUT_TYPE_DC;
                if(!Get_IEEE_1588_Mode(&mode)) {
                    if(mode == IEEE_1588_SLAVE) {
                        Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
                    }
                }
            }
            else if(strcmp(form_value, TIME_IN_SRC_ENUM_STR[TIME_INPUT_SRC_1PPS]) == 0)
            {
                time_config.source = TIME_INPUT_SRC_1PPS;
                time_config.signal = TIME_INPUT_TYPE_DC;
                if(!Get_IEEE_1588_Mode(&mode)) {
                    if(mode == IEEE_1588_SLAVE) {
                        Set_IEEE_1588_Mode(IEEE_1588_DISABLED);
                    }
                }
            }
            else if(strcmp(form_value, "TIME_SRC_1588") == 0)
            {
                time_config.source = TIME_INPUT_SRC_1588;
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else if (strcmp(form_value, "TIME_SRC_GPS0") == 0)
            {
                time_config.source = TIME_INPUT_SRC_GPS0;
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else if (strcmp(form_value, "TIME_SRC_GPS3") == 0)
            {
                time_config.source = TIME_INPUT_SRC_GPS3;
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else if (strcmp(form_value, "TIME_SRC_GPS5") == 0)
            {
                time_config.source = TIME_INPUT_SRC_GPS5;
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else if (strcmp(form_value, "TIME_SRC_DISABLED") == 0)
            {
                time_config.source = TIME_INPUT_SRC_DISABLED;
                time_config.signal = TIME_INPUT_TYPE_DC;
            }
            else
            {
                T_D("Could not find a matching form value");

            }
            T_D(form_value);

            if(Set_TimeInConfig(time_config))
            {
                T_D("Set_TimeInConfig Failed");
            }
            else
            {
                save_vns_config();
            }
            redirect(p, "/timin.htm");

        }
        else 
        {
            /* GET */
            if( TIME_INPUT_SRC_DISABLED >= time_config.source) 
            {
                snprintf(str1, sizeof(str1),",");
                snprintf(str2, sizeof(str2),TIME_IN_SRC_ENUM_STR[time_config.source]);
            } else {
                snprintf(str2, sizeof(str2),"TIME_IN_SRC... out of range,%s",TIME_IN_SRC_ENUM_STR[ TIME_INPUT_SRC_DISABLED ]);
            }
            
            strcat( str1, str2);
            snprintf(str2, sizeof(str2),"|");
            strcat( str1, str2);

            if(Get_FPGAFirmwareVersionMajor(&fw_version_major) ||
                    Get_FPGAFirmwareVersionMinor(&fw_version_minor))
            { T_E("Error occured getting firmware version\n"); }

            for(i=0; i <= TIME_INPUT_SRC_DISABLED; i++)
            {
                memset( str2, 0, 100);
                switch(i) 
                {
                    case TIME_INPUT_SRC_IRIG_B:
                    case TIME_INPUT_SRC_IRIG_A:
                    case TIME_INPUT_SRC_IRIG_G:
                        if(is_mac_ies6())
                            snprintf(str2, sizeof(str2),"%s?%s#",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i]);
                        else if(is_irig_in_able() && is_mac_ies12() )
                        {
                            if( fw_version_major >= 3 && fw_version_minor >= 1)
                                snprintf(str2, sizeof(str2),"%s?%s#",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i]);
                        }
                    case TIME_INPUT_SRC_1PPS:
                        if(is_1pps_in_able())
                        {
                            if( fw_version_major >= 3 && fw_version_minor >= 1)
                                snprintf(str2, sizeof(str2),"%s?%s#",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i]);
                        }
                        break;
                    case TIME_INPUT_SRC_1588:
                        snprintf(str2, sizeof(str2),"%s?%s#",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i]);
                        break;
                    case TIME_INPUT_SRC_GPS0:
                    case TIME_INPUT_SRC_GPS3:
                    case TIME_INPUT_SRC_GPS5:
                        if( is_mac_ies12())
                            snprintf(str2, sizeof(str2),"%s?%s %s#",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i],TIME_IN_SRC_VOLTAGE_STR[i]);
                        break;
                    case TIME_INPUT_SRC_DISABLED:
                        snprintf(str2, sizeof(str2),"%s?%s",TIME_IN_SRC_ENUM_STR[i],TIME_IN_SRC_STR[i]);
                    default:
                        break;
                }
                if(str2 != NULL)
                    strcat( str1, str2);

            }
            snprintf(str2, sizeof(str2),"|");
            strcat( str1, str2);
            send_string(p, str1);
        }

    }
    return -1;
}
static cyg_int32 handler_dot_gps_config(CYG_HTTPD_STATE* p)
{
    int form_value;
    //	vns_gps_dc_bias_t gps_config;
    vns_time_input time_in_cfg;
    if(Get_TimeInConfig(&time_in_cfg))
    {
        T_D("Get_TimeInConfig Failed");
    }
    else
    {
        if (p->method == CYG_HTTPD_METHOD_POST)
        {
            cyg_httpd_form_varable_int(p, "dotGPSBiasMode", &form_value);
            switch(form_value)
            {
                case 0:
                    time_in_cfg.source = TIME_INPUT_SRC_GPS0;
                    break;
                case 3:
                    time_in_cfg.source = TIME_INPUT_SRC_GPS3;
                    break;
                case 5:
                    time_in_cfg.source = TIME_INPUT_SRC_GPS5;
                    break;
                default:
                    T_D("Case Statement went to default");
                    break;
            }
            if(Set_TimeInConfig(time_in_cfg))
            {
                T_D("Set_TimeInConfig Failed");
            }
            redirect(p, "/dot_gps_config.htm");

        }
        else 
        {
            switch(time_in_cfg.source)
            {
                case TIME_INPUT_SRC_GPS0:
                    send_string(p, "0");
                    break;
                case TIME_INPUT_SRC_GPS3:
                    send_string(p, "3");
                    break;
                case TIME_INPUT_SRC_GPS5:
                    send_string(p, "5");
                    break;
                default:
                    send_string(p, "Error");
                    break;
            }

        }

    }
    return -1;
}

static cyg_int32 handler_dot_1588_config(CYG_HTTPD_STATE* p)
{
    int form_value;
    int rc;
    vns_1588_type ptp_mode;
    BOOL create_clock = FALSE;
    char unescaped_form_value[32];
    char* char_form_value;
    size_t len;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST)
    {

        cyg_httpd_form_varable_int(p, "dot1588Mode", &form_value);
        switch(form_value)
        {
            case 0:
                ptp_mode = IEEE_1588_MASTER;
                break;
            case 1:
                ptp_mode = IEEE_1588_SLAVE;
                Set_TimeInSource(TIME_INPUT_SRC_1588);
                break;
            case 2:
            default:
                ptp_mode = IEEE_1588_DISABLED;
                break;
        }
        char_form_value = cyg_httpd_form_varable_string(p, "override", &len);
        rc = cgi_unescape(char_form_value, unescaped_form_value, len, sizeof(unescaped_form_value));
        char_form_value = unescaped_form_value;

        if (strcmp(char_form_value, "on") == 0) {
            create_clock = TRUE;
            T_D("Override is ON! %s", char_form_value);
        }
        else {
            T_D("Override is OFF! %s", char_form_value);
        }
        if( Set_IEEE_1588_Mode_Mod_clk( ptp_mode, create_clock ) )
        {
            T_E("Set_IEEE_1588_Mode Failed");
        }
        else
        {
            save_vns_config();
        }
        redirect(p, "/dot_1588_config.htm");
    }
    else
    {
        if(Get_IEEE_1588_Mode(&ptp_mode))
        {
            send_string(p, "Error");
        }
        else
        {
            send_string( p, IEEE_1588_TYPE_STR[ptp_mode]);
        }
    }
    return -1; // Do not further search the file system.
}

static void send_string(CYG_HTTPD_STATE* p, const char *messege)
{
    T_D("Sending messege:");
    T_D(messege);
    cyg_httpd_start_chunked("html");
    strcpy(p->outbuffer, messege);
    cyg_httpd_write_chunked(p->outbuffer, strlen(p->outbuffer));
    cyg_httpd_end_chunked();
}

static cyg_int32 handler_firmware_2(CYG_HTTPD_STATE* p)
{
    form_data_t formdata[2];
    int cnt;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC))
        return -1;
#endif


    if(p->method == CYG_HTTPD_METHOD_POST) 
    {
        if((cnt = cyg_httpd_form_parse_formdata(p, formdata, ARRSZ(formdata))) > 0) {
            unsigned int i;
            form_data_t *firmware_2 = NULL;
            vtss_restart_t restart_type = VTSS_RESTART_WARM; // Default
            // Figure out which of the entries found in the POST request contains the firmware_2
            // and whether the coolstart entry exists (meaning that the checkbox is checked on the page).
            for (i = 0; i < cnt; i++) {
                if (!strcmp(formdata[i].name, "firmware_2")) {
                    firmware_2 = &formdata[i];
                } else if (!strcmp(formdata[i].name, "coolstart")) {
                    restart_type = VTSS_RESTART_COOL;
                }
            }

            if (firmware_2) {

                update_args_t update_args;
                //      update_args.buffer = firmware_2->value;
                update_args.file_length = firmware_2->value_len ;
                void *buffer;


                /* get copy of file */
                if(!(buffer  = calloc(1,update_args.file_length ))) {
                    char * err = "Allocation of firmware buffer failed";
                    T_E("%s (len %zu)", err, update_args.file_length);
                }
                /* must keep track of the beginning of the malloc for freeing. update_args.buffer
                 * will be changed to point at the file data for the FPGA
                 * */
                update_args.buffer = buffer;
                memcpy(update_args.buffer, firmware_2->value, firmware_2->value_len );

                if(fpga_firmware_check( &update_args ) != 0) {
                    if(NULL != update_args.buffer) { free(update_args.buffer); }
                    redirect(p, "/fpga_upload_invalid.htm");
                    return -1;
                }

                fpga_firmware_status_set("Configuring FPGA to receive update");

                //                T_D("update_args, buffer %X, file_size %u, cksm %X",
                //                		update_args.buffer, update_args.file_length, calc_checksum(update_args.buffer,update_args.file_length));
                //                T_D("File info: buffer %X, file_size %u, cksm %X ",
                //                		firmware_2->value, firmware_2->value_len, calc_checksum( firmware_2->value, firmware_2->value_len));

                redirect(p, "/upload_flashing_FPGA.htm");
                //                    if(UpdateVNSFPGAFirmware(&update_args)) {
                //                        char * err = "Update Failure";
                //                         T_E("%s (len %zu)", err, firmware_2->value_len);
                //                         send_custom_error(p, err, err, strlen(err));
                //                    }



                start_fpga_upgrade( &update_args);
                //TODO how to best free the memory??? update_args.buffer

                if(NULL != buffer) { free(buffer); }

            } else {
                char *err = "Firmware not found in data";
                T_E(err);
                send_custom_error(p, err, err, strlen(err));
            }
        } else {
            cyg_httpd_send_error(CYG_HTTPD_STATUS_BAD_REQUEST);
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        web_send_iolayer_output(WEB_CLI_IO_TYPE_FIRMWARE, NULL, "html");
    }
    return -1; // Do not further search the file system. 
} 
#ifdef IES_EPE_FEATURE_ENABLE
static cyg_int32 handler_dot_fpga_epe_config(CYG_HTTPD_STATE *p) 
{ 
    vtss_rc               rc = VTSS_OK; // error return code
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mirror_conf_t         conf;
    vns_epe_conf_blk_t    epe_conf;
    mirror_switch_conf_t  local_conf;
    int                   ct;
    uint32_t              epe_enable;
    char                  form_name[32];
    int                   form_value;
    port_iter_t           pit;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.


    T_D ("Mirror web access - SID =  %d", sid );

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MIRROR)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) 
    {

        strcpy(err_msg, ""); // No errors so far :)

        mirror_mgmt_conf_get(&conf); // Get the current configuration
        rc = mirror_mgmt_switch_conf_get(sid, &local_conf);
        T_D ("Updating from web");

        epe_conf.mode = VNS_EPE_DISABLE;
        epe_conf.bit_rate = VNS_EPE_PERCENT_RATE_MIN;
        // Get mirror port from WEB ( Form name = "portselect" )
        if (cyg_httpd_form_varable_int(p, "portselect", &form_value) ) {
            /* T_D ("Mirror port set to %d via web", form_value); */
            /* conf.dst_port = uport2iport(form_value); */
            conf.dst_port = VTSS_PORT_NO_NONE;
            switch(form_value) {
                case VNS_EPE_HDLC:
                    epe_conf.mode = VNS_EPE_HDLC;
                    conf.dst_port = uport2iport(VNS_EPE_PORT);
                    break;
                case VNS_EPE_CH7_15:
                    epe_conf.mode = VNS_EPE_CH7_15;
                    conf.dst_port = uport2iport(VNS_EPE_PORT);
                    break;
                case VNS_EPE_DECODE_HDLC:
                    epe_conf.mode = VNS_EPE_DECODE_HDLC;
                    break;
                case VNS_EPE_DECODE_CH7_15:
                    epe_conf.mode = VNS_EPE_DECODE_CH7_15;
                    break;
                case VNS_EPE_DISABLE:
                    break;
                default:
                    T_E ("Invalid value for 'port_select'");
            }
        } 
        else {
            /* T_D ("Mirroring disabled from web"); */
            T_E ("cyg_httpd_form_varable_int failed");
        }
        if (cyg_httpd_form_varable_int(p, "bitrate", &form_value) ) {
            switch(epe_conf.mode) {
                case VNS_EPE_HDLC:
                case VNS_EPE_CH7_15:
                    epe_conf.bit_rate = ( ( form_value * 100) / VNS_EPE_BIT_RATE_MAX );
                    break;
                case VNS_EPE_DISABLE:
                default:
                    epe_conf.bit_rate = VNS_EPE_PERCENT_RATE_MIN;
            }
            set_vns_fpga_epe_conf(epe_conf);
        } 
        else {
            /* T_D ("Mirroring disabled from web"); */
            T_E ("cyg_httpd_form_varable_int failed");
        }
        if (cyg_httpd_form_varable_int(p, "framesize", &form_value) ) {
            switch(epe_conf.mode) {
                case VNS_EPE_CH7_15:
                    epe_conf.ch7_data_word_count = form_value;
                    break;
                case VNS_EPE_HDLC:
                case VNS_EPE_DISABLE:
                default:
                    epe_conf.ch7_data_word_count = 1;
            }
            set_vns_fpga_epe_conf(epe_conf);
        } 
        else {
            /* T_D ("Mirroring disabled from web"); */
            T_E ("cyg_httpd_form_varable_int failed");
        }

        // Get mirror switch from WEB ( Form name = "switchselect" )
#if VTSS_SWITCH_STACKABLE
        if (vtss_stacking_enabled()) {
            if (cyg_httpd_form_varable_int(p, "switchselect", &form_value)) {
                T_D ("Mirror switch set to %d via web", form_value);
                conf.mirror_switch  = topo_usid2isid(form_value);
            }
        } else {
            conf.mirror_switch = VTSS_ISID_START;
        }
#endif /* VTSS_SWITCH_STACKABLE */

        // Get source and destination eanble checkbox values
        rc = mirror_mgmt_switch_conf_get(sid, &local_conf);


        // Loop through all front ports
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_OK) {
            while (port_iter_getnext(&pit)) {

                T_RG_PORT(VTSS_TRACE_GRP_DEFAULT, pit.iport, "Mirror enable configured");
                sprintf(form_name, "mode_%d", pit.uport); // Set to the htm checkbox form name
                if (cyg_httpd_form_varable_int(p, form_name, &form_value)   && (form_value >= 0 && form_value < 4)) {
                    // form_value ok
                } else {
                    form_value = 0;
                }

                if (form_value == 0) {
                    local_conf.src_enable[pit.iport] = 0;
                    local_conf.dst_enable[pit.iport] = 0;
                } else if (form_value == 1) {
                    local_conf.src_enable[pit.iport] = 1;
                    local_conf.dst_enable[pit.iport] = 0;
                } else if (form_value == 2) {
                    local_conf.src_enable[pit.iport] = 0;
                    local_conf.dst_enable[pit.iport] = 1;
                } else {
                    // form_value is 3
                    local_conf.src_enable[pit.iport] = 1;
                    local_conf.dst_enable[pit.iport] = 1;
                }
            }
        }

#ifdef VTSS_FEATURE_MIRROR_CPU
        //
        // Getting CPU configuration
        //
        if (cyg_httpd_form_varable_int(p, "mode_CPU", &form_value)   && (form_value >= 0 && form_value < 4)) {
            // form_value ok
        } else {
            form_value = 0;
        }

        if (form_value == 0) {
            local_conf.cpu_src_enable = 0;
            local_conf.cpu_dst_enable = 0;
        } else if (form_value == 1) {
            local_conf.cpu_src_enable = 1;
            local_conf.cpu_dst_enable = 0;
        } else if (form_value == 2) {
            local_conf.cpu_src_enable = 0;
            local_conf.cpu_dst_enable = 1;
        } else {
            // form_value is 3
            local_conf.cpu_src_enable = 1;
            local_conf.cpu_dst_enable = 1;
        }
#endif

        //
        // Apply new configuration.
        //
        mirror_mgmt_switch_conf_set(sid, &local_conf); // Update switch with new configuration
        // Write new configuration
        rc = mirror_mgmt_conf_set(&conf);

        redirect(p, "/dot_epe.htm");
    } 
    else 
    {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        /* ",1,0,   |1#2#3#4#5#6#7#8#9#10#11#12#13#14#?  |1/0/0,2/0/0,3/0/0,4/0/0,5/0/0,6/0/0,7/0/0,8/0/0,9/0/0,10/0/0,11/0/0,12/0/0,13/0/0,14/0/0,CPU/0/0/-,"; */
        /* ",1,13,  |HDLC#CH7#?                          |1/0/0,2/0/0,3/0/0,4/0/0,5/0/0,6/0/0,7/0/0,8/0/0,9/0/0,10/0/0,11/0/0,12/0/0/-,"; */
        /* ",1,13,  |1#2#3#4#5#6#7#8#9#10#11#12#13#14#?  |1/0/0,2/1/1,3/0/0,4/0/0,5/0/0,6/1/1,7/0/0,8/0/0,9/0/0,10/0/0,11/0/0,12/0/0,13/0/0,14/0/0,CPU/0/0/-," */
        // Transfer format = mirror switch,mirror uport,sid#sid#sid|uport/src enable/dst enable,uport/src enable/dst enable,......

        cyg_httpd_start_chunked("html");

        mirror_mgmt_conf_get(&conf); // Get the current configuration

        /* ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s,%u,%u,", */
        /*               err_msg, */
        /*               topo_isid2usid(conf.mirror_switch), */
        /*               iport2uport(conf.dst_port)); */
        if (!get_vns_fpga_epe_conf(&epe_conf) ) { 
            T_E("Could not get_vns_fpga_epe_conf.");
            return -1;
        }
        epe_enable = 0;
        if(!is_epe_able()) {
            strcpy(err_msg, "invalid"); // No errors so far :)
        }
        else if(!is_epe_license_acitive()) {
            strcpy(err_msg, "no_license"); // No errors so far :)
        }
        else if(iport2uport(conf.dst_port) == VNS_EPE_PORT || 
                iport2uport(conf.dst_port) == VNS_EPE_DISABLE) {
            epe_enable = epe_conf.mode;
        }
        else {
            strcpy(err_msg, "disable"); // No errors so far :)
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s,%u,%u,,%u,%u",
                err_msg,
                topo_isid2usid(conf.mirror_switch),
                epe_enable,
                ( ( epe_conf.bit_rate * VNS_EPE_BIT_RATE_MAX) / 100 ),
                epe_conf.ch7_data_word_count);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);

#if VTSS_SWITCH_STACKABLE
        vtss_usid_t usid;
        // Make list of SIDs
        for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
            vtss_usid_t isid = topo_usid2isid(usid);
            if (msg_switch_exists(isid)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#", usid);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
#endif /* VTSS_SWITCH_STACKABLE */


        // Insert Separator (,)
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Make list of ports that each switch in the stack have (Corresponding to the sid list above)
#if VTSS_SWITCH_STACKABLE
        for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
            vtss_usid_t isid = topo_usid2isid(usid);
            if (msg_switch_exists(isid)) {
#else
                vtss_isid_t isid = sid;
#endif
                /* if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_OK) { */
                /*     while (port_iter_getnext(&pit)) { */
                /* ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#", pit.uport); */
                /* cyg_httpd_write_chunked(p->outbuffer, ct); */
                /* } */
                /* } */

                if( is_epe_decoder_able() )
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%s#%s#%s#",
                            VNS_EPE_MODE_STR[ VNS_EPE_HDLC ],
                            VNS_EPE_MODE_STR[ VNS_EPE_CH7_15 ],
                            VNS_EPE_MODE_STR[ VNS_EPE_DECODE_HDLC ],
                            VNS_EPE_MODE_STR[ VNS_EPE_DECODE_CH7_15 ]);
                else
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%s#","HDLC","CH7-15");
                cyg_httpd_write_chunked(p->outbuffer, ct);

                // Insert Separator (?)
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "?");
                cyg_httpd_write_chunked(p->outbuffer, ct);
#if VTSS_SWITCH_STACKABLE
            }
        }
#endif


        // Get  the SID config
        rc = mirror_mgmt_switch_conf_get(sid, &local_conf);


        // Loop through all front ports
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_OK)
        {
            while (port_iter_getnext(&pit)) {
                if( pit.uport <= VNS_PORT_COUNT)
                {
                    if( VNS_PORT_COUNT == pit.uport) 
                    {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u/%u/-,",
                                pit.first ? "|" : "",
                                pit.uport,
                                local_conf.src_enable[pit.iport],
                                local_conf.dst_enable[pit.iport]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);
                    }
                    else
                    {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u/%u,",
                                pit.first ? "|" : "",
                                pit.uport,
                                local_conf.src_enable[pit.iport],
                                local_conf.dst_enable[pit.iport]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);
                    }
                }
            }
        }

#ifdef VTSS_FEATURE_MIRROR_CPU
        // CPU port
        /* ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%u/%u/%s,", */
        /*               "CPU", */
        /*               local_conf.cpu_src_enable, */
        /*               local_conf.cpu_dst_enable, */
        /*               "-" /1* Signal No trunking *1/); */
        /* cyg_httpd_write_chunked(p->outbuffer, ct); */
#endif
        cyg_httpd_end_chunked();

        strcpy(err_msg, ""); // Clear error message
    }


    if (rc != VTSS_OK)
    {
        strcpy(err_msg, mirror_error_txt(rc));
    }
    return -1; // Do not further search the file system.
}
#endif /* IES_EPE_FEATURE_ENABLE */

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_fpga_upload_invalid, "/config/fpga_upload_invalid", handler_fpga_upload_invalid);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_fpga_firmware_status, "/config/fpga_firmware_status", handler_fpga_firmware_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_firmware_2, "/config/firmware_2", handler_firmware_2);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_1588_config, "/config/dot_1588_config", handler_dot_1588_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_gps_config, "/config/dot_gps_config", handler_dot_gps_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_timin_config, "/config/dot_timin_config", handler_dot_timin_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_timout_config, "/config/dot_timout_config", handler_dot_timout_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_fpga_status, "/config/dot_fpga_status", handler_dot_fpga_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_discrete_config, "/config/dot_discrete_config", handler_dot_fpga_discrete_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_discrete_trigger, "/config/dot_discrete_trigger", handler_dot_fpga_discrete_trigger);
#ifdef IES_EPE_FEATURE_ENABLE
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_fpga_epe_config, "/config/dot_fpga_epe_config", handler_dot_fpga_epe_config);
#endif /* IES_EPE_FEATURE_ENABLE */
/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
