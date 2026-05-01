///*
//
//   Vitesse Switch API software.
//
// Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
// Rights Reserved.
//
// Unpublished rights reserved under the copyright laws of the United States of
// America, other countries and international treaties. Permission to use, copy,
// store and modify, the software and its source code is granted. Permission to
// integrate into other products, disclose, transmit and distribute the software
// in an absolute machine readable format (e.g. HEX file) is also granted.  The
// source code of the software may not be disclosed, transmitted or distributed
// without the written permission of Vitesse. The software and its source code
// may only be used in products utilizing the Vitesse switch products.
//
// This copyright notice must appear in any copy, modification, disclosure,
// transmission or distribution of the software. Vitesse retains all ownership,
// copyright, trade secret and proprietary rights in the software.
//
// THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
// INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR USE AND NON-INFRINGEMENT.
// */
//
//#include "web_api.h"
//#ifdef VTSS_SW_OPTION_PRIV_LVL
//#include "vtss_privilege_api.h"
//#include "vtss_privilege_web_api.h"
//#endif
//#include "flash_mgmt_api.h"
//#include "firmware_api.h"
//#include "mitr_fpga_api.h"
//#include "mitr_fpga_cli.h"   /* Check that public function decls. and defs. are in sync 	*/
//#include "mitr_types.h"
//
///* =================
// * Trace definitions
// * -------------- */
//#include <vtss_module_id.h>
//#include <vtss_trace_lvl_api.h>
//#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MITR_FPGA
//#include <vtss_trace_api.h>
///* ============== */
//
//
//#define JASONS_TESTING 1
//#define CONT_WORK 1
//
//
//static void send_string(CYG_HTTPD_STATE* p, const char *messege);
//
//
///****************************************************************************/
///*  Web Handler  functions                                                  */
///****************************************************************************/
//
//static cyg_int32 handler_dot_fpga_status(CYG_HTTPD_STATE* p)
//{
//	// **** To create string to pass to client
//		char str1[1000];
//		char str2[100];
////		char str3[12];
////		int  len ;
//		uint32_t temp;
//	// **** For Getting time
//		int year, yday, hour,minute, second;
////		mitr_time_input_src_t time_in_src;
//		mitr_1588_type ieee1588mode;
//	// **** For retrieving status
//		mitr_status_t status;
//		int percent_complete;
//		int warning_bits = 0; // not sure
//		int error_bits = 0; // count number of health bits set (all systems, I think)
//		int i;
//
////*********************	SYSTEM Time	****************************//
//		snprintf(str1, sizeof(str1),".TIME,");
//		if(0 == Get_IEEE_1588_Mode(&ieee1588mode))
//		{
//			switch(ieee1588mode)
//			{
//			case MITR_IEEE_1588_SLAVE:
//				if(0 == Get_PTP_SetTime(&year, &yday, &hour, &minute, &second))
//				{
//					snprintf(str2, sizeof(str2),"Time In (1588):  %03d-%02d:%02d:%02d,", yday, hour, minute, second);
//
//				}
//				break;
//			default:
//				if(0 == Get_TimeIn_Time(&year, &yday, &hour, &minute, &second))
//				{
//					snprintf(str2, sizeof(str2),"Time In:  %03d-%02d:%02d:%02d,", yday, hour, minute, second);
//				}
//				break;
//
//			}
//			strcat( str1, str2);
//			if(0 == Get_TimeOut_Time(&year, &yday, &hour, &minute, &second))
//			{
//				snprintf(str2, sizeof(str2),"Time Out: %03d-%02d:%02d:%02d,", yday, hour, minute, second);
//				strcat( str1, str2);
//			}
//		}
//		else
//		{
//			snprintf(str2, sizeof(str2),"Time In: Error,Time Out: Error,");//, yday, hour, minute, second);
//			strcat( str1, str2);
//		}
//
////*********************	STATUS cmd	****************************//
//
//		GetSystemHealthReg(&temp);
//		for(i = 0; i < 32; i++)
//		{
//			if( ( (temp >> i) & 0x01) )
//				error_bits++;
//		}
//		GetDiscreteInHealthReg(&temp);
//		for(i = 0; i < 32; i++)
//		{
//			if( ( (temp >> i) & 0x01) )
//				error_bits++;
//		}
//		GetDiscreteOutHealthReg(&temp);
//		for(i = 0; i < 32; i++)
//		{
//			if( ( (temp >> i) & 0x01) )
//				error_bits++;
//		}
//
//		GetTimeInHealthReg(&temp);
//		for(i = 0; i < 32; i++)
//		{
//			if( ( (temp >> i) & 0x01) )
//				error_bits++;
//		}
//		GetTimeOutHealthReg(&temp);
//		for(i = 0; i < 32; i++)
//		{
//			if( ( (temp >> i) & 0x01) )
//				error_bits++;
//		}
//		/* FINALLY, GET STATUS */
//
//		status = GetStatus();
//		snprintf(str2, sizeof(str2),".STATUS,");
//		strcat( str1, str2);
//		snprintf(str2, sizeof(str2),"S %02d %d %d,", (int)status, warning_bits, error_bits);
//		strcat( str1, str2);
//		switch(status)
//		{
//		case MITR_STATE_BIT:
//			if(GetPercentComplete(&percent_complete) == 0)
//			{
//				snprintf(str2, sizeof(str2)," %d%%,", percent_complete);
//				strcat( str1, str2);
//			}
//			break;
//		case MITR_STATE_BUSY:
//			if(GetPercentComplete(&percent_complete) == 0)
//			{
//				snprintf(str2, sizeof(str2)," %d%%,", percent_complete);
//				strcat( str1, str2);
//			}
//			break;
//		default:
//			break;
//		}
////		if(verbose)
//			if((status >= MITR_STATE_FAIL) && (status < MITR_STATE_UNKNOWN))
//				snprintf(str2, sizeof(str2)," %s,", mitr_status_name[status]);
///*			else
//				CPRINTF("\n");
//		else
//			CPRINTF("\n");
//*/
////*********************	SYSTEM health	****************************//
//	   GetSystemHealthReg(&temp);
//	   snprintf(str2, sizeof(str2),".SYSTEM Error(s),");
//	   strcat( str1, str2);
//		if(temp & HEALTH_BIT_FAIL)
//		{
//			snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
//			strcat( str1, str2);
//		}
//		if(temp & HEALTH_SETUP_FAIL)
//		{
//			snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
//			strcat( str1, str2);
//		}
//		if(temp & SYS_HEALTH_OPERATION_FAIL)
//		{
//			snprintf(str2, sizeof(str2),"%08X Operation Failure,", SYS_HEALTH_OPERATION_FAIL);
//			strcat( str1, str2);
//		}
//		if(temp & SYS_HEALTH_BUSY)
//		{
//			snprintf(str2, sizeof(str2),"%08X Busy,", SYS_HEALTH_BUSY);
//			strcat( str1, str2);
//		}
//		if(temp & SYS_HEALTH_POWER_BOARD_OVER_TEMP)
//		{
//			snprintf(str2, sizeof(str2),"%08X Power Board Over-temp,", SYS_HEALTH_POWER_BOARD_OVER_TEMP);
//			strcat( str1, str2);
//		}
//		if(temp & SYS_HEALTH_FPGA_BOARD_PSC_FAIL)
//		{
//			snprintf(str2, sizeof(str2),"%08X Power System Controller Failure,", SYS_HEALTH_FPGA_BOARD_PSC_FAIL);
//			strcat( str1, str2);
//		}
//		if(temp & SYS_HEALTH_GPS_FAULT)
//		{
//			snprintf(str2, sizeof(str2),"%08X GPS Fault,", SYS_HEALTH_GPS_FAULT);
//			strcat( str1, str2);
//		}
//
////*********************	TIME IN	****************************//
////
////	   GetTimeInHealthReg(&temp);
////	   snprintf(str2, sizeof(str2),".TIME IN Error(s),");
////	   strcat( str1, str2);
////		if(temp & HEALTH_BIT_FAIL)
////		{
////			snprintf(str2, sizeof(str2),"%08X BIT Failure,", HEALTH_BIT_FAIL);
////			strcat( str1, str2);
////		}
////		if(temp & HEALTH_SETUP_FAIL)
////		{
////			snprintf(str2, sizeof(str2),"%08X Setup Failure,", HEALTH_SETUP_FAIL);
////			strcat( str1, str2);
////		}
////		if(temp & TI_HEALTH_NO_SIGNAL)
////		{
////			snprintf(str2, sizeof(str2),"%08X No Signal,", TI_HEALTH_NO_SIGNAL);
////			strcat( str1, str2);
////		}
////		if(temp & TI_HEALTH_BAD_SIGNAL)
////		{
////			snprintf(str2, sizeof(str2),"%08X Bad Signal,", TI_HEALTH_BAD_SIGNAL);
////			strcat( str1, str2);
////		}
////		if(temp & TI_HEALTH_SYNC_FAIL)
////		{
////			snprintf(str2, sizeof(str2),"%08X Sync Failure,", TI_HEALTH_SYNC_FAIL);
////			strcat( str1, str2);
////		}
//
//	   /* concatenates str1 and str2 */
//
//	   send_string(p, str1);
//
//
//	return -1;
//}
//
//
//static cyg_int32 handler_dot_1588_config(CYG_HTTPD_STATE* p)
//{
//	int form_value;
//	mitr_1588_type ptp_mode;
//#ifdef VTSS_SW_OPTION_PRIV_LVL
//    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
//        return -1;
//    }
//#endif
//
//    if (p->method == CYG_HTTPD_METHOD_POST)
//    {
//
//    	cyg_httpd_form_varable_int(p, "dot1588Mode", &form_value);
//		switch(form_value)
//		{
//		case 0:
//			ptp_mode = MITR_IEEE_1588_MASTER;
//			break;
//		case 1:
//			ptp_mode = MITR_IEEE_1588_SLAVE;
//			break;
//		case 2:
//			ptp_mode = MITR_IEEE_1588_DISABLED;
//			break;
//		default:
//			break;
//		}
////		if(Set_IEEE_1588_Mode(ptp_mode))
////		{
////			T_D("Set_IEEE_1588_Mode Failed");
////		}
//        redirect(p, "/dot_1588_config.htm");
//    }
//
//	if(Get_IEEE_1588_Mode(&ptp_mode))
//	{
//		send_string(p, "Error");
//	}
//	else
//	{
//		send_string( p, IEEE_1588_TYPE_STR[ptp_mode]);
//	}
//
//
//    return -1; // Do not further search the file system.
//}
//
//static void send_string(CYG_HTTPD_STATE* p, const char *messege)
//{
//	T_D("Sending messege:");
//	T_D(messege);
//	cyg_httpd_start_chunked("html");
//	strcpy(p->outbuffer, messege);
//	cyg_httpd_write_chunked(p->outbuffer, strlen(p->outbuffer));
//	cyg_httpd_end_chunked();
//}
////
////static cyg_int32 handler_firmware_2(CYG_HTTPD_STATE* p)
////{
////    form_data_t formdata[2];
////    int cnt;
////
////
////#ifdef VTSS_SW_OPTION_PRIV_LVL
////    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC))
////        return -1;
////#endif
////
////
////    if(p->method == CYG_HTTPD_METHOD_POST) {
////        if((cnt = cyg_httpd_form_parse_formdata(p, formdata, ARRSZ(formdata))) > 0) {
////            int i;
////            form_data_t *firmware_2 = NULL;
////            vtss_restart_t restart_type = VTSS_RESTART_WARM; // Default
////            // Figure out which of the entries found in the POST request contains the firmware_2
////            // and whether the coolstart entry exists (meaning that the checkbox is checked on the page).
////            for (i = 0; i < cnt; i++) {
////                if (!strcmp(formdata[i].name, "firmware_2")) {
////                    firmware_2 = &formdata[i];
////                } else if (!strcmp(formdata[i].name, "coolstart")) {
////                    restart_type = VTSS_RESTART_COOL;
////                }
////            }
////
////            if (firmware_2) {
//////                unsigned long image_version;
////                unsigned char *buffer;
////                /* NB: We malloc and copy to ensure proper aligment of image! */
////                if((buffer = malloc(firmware_2->value_len))) {
////                    memcpy(buffer, firmware_2->value, firmware_2->value_len);
////                    firmware_status_set(buffer); //
////                      redirect(p, "/upload_flashing.htm");
////
////                 }
////
////            }
////            else{
////             	return -1; // Do not further search the file system.
////             }
////        } else {
////            cyg_httpd_send_error(CYG_HTTPD_STATUS_BAD_REQUEST);
////        }
////	} else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
////		web_send_iolayer_output(WEB_CLI_IO_TYPE_FIRMWARE, NULL, "html");
////	}
////
////    return 0;
////}
//
//
//
//
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_firmware_2, "/config/firmware_2", handler_firmware_2);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_1588_config, "/config/dot_1588_config", handler_dot_1588_config);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_gps_config, "/config/dot_gps_config", handler_dot_gps_config);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_timin_config, "/config/dot_timin_config", handler_dot_timin_config);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_timout_config, "/config/dot_timout_config", handler_dot_timout_config);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_fpga_status, "/config/dot_fpga_status", handler_dot_fpga_status);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_discrete_config, "/config/dot_discrete_config", handler_dot_fpga_discrete_config);
////CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_FPGA_dot_discrete_trigger, "/config/dot_discrete_trigger", handler_dot_fpga_discrete_trigger);
//
//
