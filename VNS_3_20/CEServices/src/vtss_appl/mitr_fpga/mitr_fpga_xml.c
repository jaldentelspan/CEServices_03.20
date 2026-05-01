///*
//
// Vitesse Switch API software.
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
//
//*/
//
//#include "main.h"
//
//
//#include "conf_xml_api.h"
//#include "conf_xml_trace_def.h"
//#include "misc_api.h"
//#include "mgmt_api.h"
//
//#include "mitr_fpga_api.h"
//
///* Tag IDs */
//enum {
//    /* Module tags */
//    CX_TAG_MITR_FPGA,
//
//    /* Group tags */
//    CX_TAG_MITR_FPGA_CONF_STRUCT,
//
//    /* Parameter tags */
//    CX_TAG_ENTRY,
//
//    /* Last entry */
//    CX_TAG_NONE
//};
//
///* Tag table */
//static cx_tag_entry_t mitr_fpga_cx_tag_table[CX_TAG_NONE + 1] =
//{
//    [CX_TAG_MITR_FPGA] = {
//        .name  = "mitr_fpga",
//        .descr = "MITR FPGA",
//        .type = CX_TAG_TYPE_MODULE
//    },
//
//    [CX_TAG_MITR_FPGA_CONF_STRUCT] = {
//        .name  = "mitr_fpga_conf_struct",
//        .descr = "",
//        .type = CX_TAG_TYPE_GROUP
//    },
//    [CX_TAG_ENTRY] = {
//        .name  = "entry",
//        .descr = "",
//        .type = CX_TAG_TYPE_PARM
//    },
//
//    /* Last entry */
//    [CX_TAG_NONE] = {
//        .name  = "",
//        .descr = "",
//        .type = CX_TAG_TYPE_NONE
//    }
//};
//
//static vtss_rc mitr_fpga_cx_parse_func(cx_set_state_t *s)
//{
//	int err;
//    switch (s->cmd) {
//    case CX_PARSE_CMD_PARM:
//    {
//        BOOL           global;
//
//        global = (s->isid == VTSS_ISID_GLOBAL);
//        if (global) {
//            s->ignored = 1;
//            break;
//        }
//
//        switch (s->id) {
//        case CX_TAG_MITR_FPGA_CONF_STRUCT:
//            break;
//        case CX_TAG_ENTRY:
//        		{
//        			config_transport_t conf;
//        			get_mitr_fpga_master_conf_size(&conf);
//        			struct_to_string_size_cal(&conf);
//
//        			char hex_string[conf.hex_string.size];
//        			conf.hex_string.pointer.c = hex_string;
//
//        			if(cx_parse_txt(s, "conf_struct", conf.hex_string.pointer.c, conf.hex_string.size) != VTSS_OK) {
//        				return VTSS_UNSPECIFIED_ERROR;
//        			}
//
//        			unsigned char byte_array[conf.data.size];
//        			conf.data.pointer.uc = byte_array;
//        			err = hexstring_to_struct(&conf );
//        			if(0 != err) {
//        				return VTSS_UNSPECIFIED_ERROR;
//        			}
//
//        			if(!set_mitr_fpga_master_config(&conf)) {
//          				return VTSS_UNSPECIFIED_ERROR;
//        			}
//        		}
//
//            break;
//        default:
//            s->ignored = 1;
//            break;
//        }
//        break;
//    } /* CX_PARSE_CMD_PARM */
//    case CX_PARSE_CMD_GLOBAL:
//
//        break;
//    case CX_PARSE_CMD_SWITCH:
//
//        break;
//    default:
//
//        break;
//    }
//
//    return s->rc;
//}
//
//static vtss_rc mitr_fpga_cx_gen_func(cx_get_state_t *s)
//{
//    switch (s->cmd) {
//    case CX_GEN_CMD_GLOBAL:
//
//        break;
//    case CX_GEN_CMD_SWITCH:
//    	cx_add_tag_line(s, CX_TAG_MITR_FPGA, 0);
//    	{
//    		cx_add_attr_start(s, CX_TAG_ENTRY);
//
//    		config_transport_t conf;
//    		get_mitr_fpga_master_conf_size(&conf);
//
//    		unsigned char conf_buf[conf.data.size];
//    		conf.data.pointer.uc = conf_buf;
//
//    		if(!get_mitr_fpga_master_config(&conf)) {
//    			return VTSS_UNSPECIFIED_ERROR;
//    		}
//
//    		struct_to_string_size_cal(&conf);
//    		char string_buf[conf.hex_string.size];
//    		conf.hex_string.pointer.c = string_buf;
//
//    		struct_to_string(&conf);
//
//    		cx_add_attr_txt(s, "conf_struct", conf.hex_string.pointer.c);
//    		CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
//
//    	}
//    	CX_RC(cx_add_tag_line(s, CX_TAG_MITR_FPGA, 1));
//        break;
//    default:
//        T_E("Unknown command");
//        return VTSS_RC_ERROR;
//        break;
//    } /* End of Switch */
//
//    return VTSS_OK;
//}
//
///* Register the info in to the cx_module_table */
//CX_MODULE_TAB_ENTRY(
//    VTSS_MODULE_ID_MITR_FPGA,
//	mitr_fpga_cx_tag_table,
//    0,
//    0,
//    NULL,                /* init function       */
//	mitr_fpga_cx_gen_func,     /* Generation fucntion */
//	mitr_fpga_cx_parse_func    /* parse fucntion      */
//);
//
