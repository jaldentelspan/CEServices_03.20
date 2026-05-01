#ifndef _VTSS_IES_BOARD_CONF_H_
#define _VTSS_IES_BOARD_CONF_H_

/* #include "ies_board_conf.h" */



#define IES_FLASH_CONF_ITEMS (16)


typedef enum{
    FLASH_CONF_POWER_BRD_TAG = 0,
    FLASH_CONF_POWER_REV_BRD_TAG,
    FLASH_CONF_FPGA_BRD_TAG,
    FLASH_CONF_FPGA_REV_BRD_TAG,
    FLASH_CONF_SWITCH_BRD_TAG,
    FLASH_CONF_SWITCH_REV_BRD_TAG,
    FLASH_CONF_INTERCONNECT_BRD_TAG,
    FLASH_CONF_INTERCONNECT_REV_BRD_TAG,
    FLASH_CONF_UNIT_SERIAL_NUM_TAG,
    FLASH_CONF_UNIT_MODEL_TAG,
    FLASH_CONF_EPE_LIC_0_TAG,
    FLASH_CONF_EPE_LIC_1_TAG,
    FLASH_CONF_EPE_LIC_2_TAG,
    FLASH_CONF_EPE_LIC_3_TAG,
    FLASH_CONF_EPE_LIC_4_TAG,

    /* add above here */
    FLASH_CONF_END_TAG,
} flash_conf_tags_t;

typedef struct flash_conf_tag_string {
	char *tag;
}flash_conf_tag_string_t;
/* need to record board part number */
extern const flash_conf_tag_string_t flash_conf_tag[];

typedef enum{
    /* default value is zero; this means a value has not been set */
    FLASH_CONF_BRD_NOT_ENTERED = 0,
    FLASH_CONF_BRD_MODEL_DRS_VNS_12,
    FLASH_CONF_BRD_MODEL_DRS_VNS_6,
    FLASH_CONF_BRD_MODEL_IES_1275_12,
    FLASH_CONF_BRD_MODEL_DRS_VNS_8,
    FLASH_CONF_BRD_MODEL_DRS_VNS_16,

    /* add above here */
    FLASH_CONF_MODEL_END,
} flash_conf_board_model_t;

typedef struct flash_conf_model_string {
	char *model;
}flash_conf_model_string_t;
extern const flash_conf_model_string_t flash_conf_model[];

#endif /* _VTSS_IES_BOARD_CONF_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/

