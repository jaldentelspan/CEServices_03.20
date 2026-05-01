/*
 *
 *  tsd_remote_update_brdg.c
 *
 * Created on: 2019-12-11 14:25
 *     Author: jalden
 *
 */

#include "tsd_register_common.h"
#include "tsd_remote_update_brdg.h"

const char * tsd_error_code_string[] = {
    [ RU_ERROR_CODE_SUCCESS ]          = "RU_ERROR_CODE_SUCCESS",
    [ RU_ERROR_CODE_ERASE_FAILURE ]    = "RU_ERROR_CODE_ERASE_FAILURE",
    [ RU_ERROR_CODE_INVALID_BOARD_ID ] = "RU_ERROR_CODE_INVALID_BOARD_ID",
    [ RU_ERROR_CODE_INVALID_HEADER ]   = "RU_ERROR_CODE_INVALID_HEADER",
    [ RU_ERROR_CODE_DATA_CHKSUM_FAIL ] = "RU_ERROR_CODE_DATA_CHKSUM_FAIL",
    [ RU_ERROR_CODE_IMAGE_TOO_BIG ]    = "RU_ERROR_CODE_IMAGE_TOO_BIG",
    [ RU_ERROR_CODE_MISSED_CMD ]       = "RU_ERROR_CODE_MISSED_CMD",
    [ RU_ERROR_CODE_MISSED_DATA ]      = "RU_ERROR_CODE_MISSED_DATA",
    [ RU_ERROR_CODE_OTHER_FAIL ]       = "RU_ERROR_CODE_OTHER_FAIL"
};

///////////////////////////////////////////////////////////////////////////
//// Function Definitions
///////////////////////////////////////////////////////////////////////////

int get_tsd_remote_update_brdg_date_code_l_seconds( uint32_t base, uint32_t *seconds )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_SECONDS_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_SECONDS_SHFT,
        seconds );
}


int get_tsd_remote_update_brdg_date_code_l_minutes( uint32_t base, uint32_t *minutes )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_MINUTES_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_MINUTES_SHFT,
        minutes );
}


int get_tsd_remote_update_brdg_date_code_l_hours( uint32_t base, uint32_t *hours )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_HOURS_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_HOURS_SHFT,
        hours );
}


int get_tsd_remote_update_brdg_date_code_l_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_date_code_h_day( uint32_t base, uint32_t *day )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_DAY_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_DAY_SHFT,
        day );
}


int get_tsd_remote_update_brdg_date_code_h_month( uint32_t base, uint32_t *month )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_MONTH_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_MONTH_SHFT,
        month );
}


int get_tsd_remote_update_brdg_date_code_h_year( uint32_t base, uint32_t *year )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_YEAR_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_YEAR_SHFT,
        year );
}


int get_tsd_remote_update_brdg_date_code_h_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_ctrl_copy_app_2_factory( uint32_t base, uint32_t *copy_app_2_factory )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_SHFT,
        copy_app_2_factory );
}

int set_tsd_remote_update_brdg_ctrl_copy_app_2_factory( uint32_t base, uint32_t copy_app_2_factory )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_SHFT,
        copy_app_2_factory );
}


int get_tsd_remote_update_brdg_ctrl_erase_app_img( uint32_t base, uint32_t *erase_app_img )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_SHFT,
        erase_app_img );
}

int set_tsd_remote_update_brdg_ctrl_erase_app_img( uint32_t base, uint32_t erase_app_img )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_SHFT,
        erase_app_img );
}


int get_tsd_remote_update_brdg_ctrl_start_update( uint32_t base, uint32_t *start_update )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_SHFT,
        start_update );
}

int set_tsd_remote_update_brdg_ctrl_start_update( uint32_t base, uint32_t start_update )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_SHFT,
        start_update );
}


int get_tsd_remote_update_brdg_ctrl_load_fctry_img( uint32_t base, uint32_t *load_fctry_img )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_SHFT,
        load_fctry_img );
}

int set_tsd_remote_update_brdg_ctrl_load_fctry_img( uint32_t base, uint32_t load_fctry_img )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_SHFT,
        load_fctry_img );
}


int get_tsd_remote_update_brdg_ctrl_veryify_header( uint32_t base, uint32_t *veryify_header )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_SHFT,
        veryify_header );
}

int set_tsd_remote_update_brdg_ctrl_veryify_header( uint32_t base, uint32_t veryify_header )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_CTRL_REG,
        TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_MASK,
        TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_SHFT,
        veryify_header );
}


int get_tsd_remote_update_brdg_data_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_data_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_DATA_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_l26_time_tick_direction_l26_time_tick( uint32_t base, uint32_t *l26_time_tick )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_REG,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_MASK,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_SHFT,
        l26_time_tick );
}

int set_tsd_remote_update_brdg_l26_time_tick_direction_l26_time_tick( uint32_t base, uint32_t l26_time_tick )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_REG,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_MASK,
        TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_SHFT,
        l26_time_tick );
}


int get_tsd_remote_update_brdg_status_percent_done( uint32_t base, uint32_t *percent_done )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_PERCENT_DONE_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_PERCENT_DONE_SHFT,
        percent_done );
}


int get_tsd_remote_update_brdg_status_flash_copy( uint32_t base, uint32_t *flash_copy )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_FLASH_COPY_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_FLASH_COPY_SHFT,
        flash_copy );
}


int get_tsd_remote_update_brdg_status_error_code( uint32_t base, uint32_t *error_code )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_ERROR_CODE_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_ERROR_CODE_SHFT,
        error_code );
}


int get_tsd_remote_update_brdg_status_verify( uint32_t base, uint32_t *verify )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_VERIFY_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_VERIFY_SHFT,
        verify );
}


int get_tsd_remote_update_brdg_status_factory( uint32_t base, uint32_t *factory )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_FACTORY_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_FACTORY_SHFT,
        factory );
}


int get_tsd_remote_update_brdg_status_busy( uint32_t base, uint32_t *busy )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_BUSY_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_BUSY_SHFT,
        busy );
}


int get_tsd_remote_update_brdg_status_erasing( uint32_t base, uint32_t *erasing )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_STATUS_REG,
        TSD_REMOTE_UPDATE_BRDG_STATUS_ERASING_MASK,
        TSD_REMOTE_UPDATE_BRDG_STATUS_ERASING_SHFT,
        erasing );
}


int get_tsd_remote_update_brdg_front_panel_brd_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_front_panel_brd_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_front_panel_brd_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_front_panel_brd_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_front_panel_brd_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_front_panel_brd_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_switch_brd_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_switch_brd_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_switch_brd_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_switch_brd_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_switch_brd_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_switch_brd_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_fpga_brd_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_fpga_brd_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_fpga_brd_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_fpga_brd_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_fpga_brd_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_fpga_brd_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_power_brd_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_power_brd_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_power_brd_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_power_brd_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_power_brd_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_power_brd_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_serial_num_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_serial_num_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_serial_num_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_serial_num_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_serial_num_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_serial_num_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_model_num_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_model_num_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_model_num_revision( uint32_t base, uint32_t *revision )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_SHFT,
        revision );
}

int set_tsd_remote_update_brdg_model_num_revision( uint32_t base, uint32_t revision )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_SHFT,
        revision );
}


int get_tsd_remote_update_brdg_model_num_brd_number( uint32_t base, uint32_t *brd_number )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_SHFT,
        brd_number );
}

int set_tsd_remote_update_brdg_model_num_brd_number( uint32_t base, uint32_t brd_number )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_MASK,
        TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_SHFT,
        brd_number );
}


int get_tsd_remote_update_brdg_scratch_1_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_scratch_1_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_scratch_2_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_scratch_2_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_scratch_3_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_scratch_3_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_SHFT,
        reg );
}


int get_tsd_remote_update_brdg_scratch_4_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_SHFT,
        reg );
}

int set_tsd_remote_update_brdg_scratch_4_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_MASK,
        TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_SHFT,
        reg );
}


