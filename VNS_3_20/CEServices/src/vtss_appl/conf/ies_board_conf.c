
#include "ies_board_conf.h"



const flash_conf_tag_string_t flash_conf_tag[] = {
     [ FLASH_CONF_POWER_BRD_TAG ]              = {"POWER"},
     [ FLASH_CONF_POWER_REV_BRD_TAG ]          = {"POWER_REV"},
     [ FLASH_CONF_FPGA_BRD_TAG ]               = {"FPGA"},
     [ FLASH_CONF_FPGA_REV_BRD_TAG ]           = {"FPGA_REV"},
     [ FLASH_CONF_SWITCH_BRD_TAG ]             = {"SWITCH"},
     [ FLASH_CONF_SWITCH_REV_BRD_TAG ]         = {"SWITCH_REV"},
     [ FLASH_CONF_INTERCONNECT_BRD_TAG ]       = {"INTERCONNECT"},
     [ FLASH_CONF_INTERCONNECT_REV_BRD_TAG ]   = {"INTERCNT_REV"},
     [ FLASH_CONF_UNIT_SERIAL_NUM_TAG ]        = {"UNITSN"},
     [ FLASH_CONF_UNIT_MODEL_TAG ]             = {"MODEL"},
     [ FLASH_CONF_EPE_LIC_0_TAG ]              = {"EPELIC0"},
     [ FLASH_CONF_EPE_LIC_1_TAG ]              = {"EPELIC1"},
     [ FLASH_CONF_EPE_LIC_2_TAG ]              = {"EPELIC2"},
     [ FLASH_CONF_EPE_LIC_3_TAG ]              = {"EPELIC3"},
     [ FLASH_CONF_EPE_LIC_4_TAG ]              = {"EPELIC4"},

    /* add above here */
     [ FLASH_CONF_END_TAG ]                    = {""}
};
const flash_conf_model_string_t flash_conf_model[] = {
     [ FLASH_CONF_BRD_NOT_ENTERED ]         = {"Board MODEL has not been entered"},
     [ FLASH_CONF_BRD_MODEL_DRS_VNS_12 ]    = {"DRS-VNS-12 12-PORT SWITCH"},
     [ FLASH_CONF_BRD_MODEL_DRS_VNS_6 ]     = {"DRS-VNS-1 6-PORT SWITCH"},
     [ FLASH_CONF_BRD_MODEL_IES_1275_12 ]   = {"iES-12-1275-1 12-PORT SWITCH"},
     [ FLASH_CONF_BRD_MODEL_DRS_VNS_8 ]     = {"DRS-VNS-8 8-PORT SWITCH"},
     [ FLASH_CONF_BRD_MODEL_DRS_VNS_16 ]    = {"DRS-VNS-16 16-PORT SWITCH"},

    /* add above here */
     [ FLASH_CONF_MODEL_END ]               = {""}
};

