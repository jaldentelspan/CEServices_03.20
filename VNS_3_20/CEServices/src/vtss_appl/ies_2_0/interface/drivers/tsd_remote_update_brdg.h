/*
 *
 *  tsd_remote_update_brdg.h
 *
 * Created on: 2019-12-11 14:25
 *     Author: jalden
 *
 */


#ifndef __TSD_REMOTE_UPDATE_BRDG_H__
#define __TSD_REMOTE_UPDATE_BRDG_H__

#include <time.h>
#include <stddef.h>
/* #include "stdint.h" */
#include "main.h"
#include <io.h>
#include "tsd_remote_update_brdg_reg.h"


#define TSD_REMOTE_UPDATE_BRDG_DRIVER_VERSION (0.01)



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef enum {
    TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD = TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG,
    TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD      = TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG,
    TSD_REMOTE_UPDATE_BRDG_FPGA_BRD        = TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG,
    TSD_REMOTE_UPDATE_BRDG_POWER_BRD       = TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG,
    TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM      = TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG,
    TSD_REMOTE_UPDATE_BRDG_MODEL_NUM       = TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG,
    TSD_REMOTE_UPDATE_BRDG_SCRATCH_1       = TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG,
    TSD_REMOTE_UPDATE_BRDG_SCRATCH_2       = TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG,
    TSD_REMOTE_UPDATE_BRDG_SCRATCH_3       = TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG,
    TSD_REMOTE_UPDATE_BRDG_SCRATCH_4       = TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG
}tsd_scratch_register_name;


typedef enum{
    RU_ERROR_CODE_SUCCESS = 0,
    RU_ERROR_CODE_ERASE_FAILURE,
    RU_ERROR_CODE_INVALID_BOARD_ID,
    RU_ERROR_CODE_INVALID_HEADER,
    RU_ERROR_CODE_DATA_CHKSUM_FAIL,
    RU_ERROR_CODE_IMAGE_TOO_BIG,
    RU_ERROR_CODE_MISSED_CMD,
    RU_ERROR_CODE_MISSED_DATA,
    RU_ERROR_CODE_OTHER_FAIL
}ru_error_code_t;

extern const char *tsd_error_code_string[];

#define TSD_L26_TIME_TICK_OUTPUT (1)
#define TSD_L26_TIME_TICK_INPUT  (0)

#define TSD_REMOTE_UPDATE_ERASING  (1)
#define TSD_REMOTE_UPDATE_ERASING_DONE  (0)

#define TSD_REMOTE_UPDATE_VERIFYING  (1)
#define TSD_REMOTE_UPDATE_VERIFYING_DONE  (0)

#define TSD_REMOTE_UPDATE_BUSY  (1)
#define TSD_REMOTE_UPDATE_NOT_BUSY  (0)

#define TSD_FPGA_UPDATE_HEADER_SIZE			( 32 )
///////////////////////////////////////////////////////////////////////////
//// Function Prototypes
///////////////////////////////////////////////////////////////////////////

int get_tsd_remote_update_brdg_date_code_l_seconds( uint32_t base, uint32_t *seconds );

int get_tsd_remote_update_brdg_date_code_l_minutes( uint32_t base, uint32_t *minutes );

int get_tsd_remote_update_brdg_date_code_l_hours( uint32_t base, uint32_t *hours );

int get_tsd_remote_update_brdg_date_code_l_reg( uint32_t base, uint32_t *reg );

int get_tsd_remote_update_brdg_date_code_h_day( uint32_t base, uint32_t *day );

int get_tsd_remote_update_brdg_date_code_h_month( uint32_t base, uint32_t *month );

int get_tsd_remote_update_brdg_date_code_h_year( uint32_t base, uint32_t *year );

int get_tsd_remote_update_brdg_date_code_h_reg( uint32_t base, uint32_t *reg );

int get_tsd_remote_update_brdg_ctrl_copy_app_2_factory( uint32_t base, uint32_t *copy_app_2_factory );
int set_tsd_remote_update_brdg_ctrl_copy_app_2_factory( uint32_t base, uint32_t copy_app_2_factory );

int get_tsd_remote_update_brdg_ctrl_erase_app_img( uint32_t base, uint32_t *erase_app_img );
int set_tsd_remote_update_brdg_ctrl_erase_app_img( uint32_t base, uint32_t erase_app_img );

int get_tsd_remote_update_brdg_ctrl_start_update( uint32_t base, uint32_t *start_update );
int set_tsd_remote_update_brdg_ctrl_start_update( uint32_t base, uint32_t start_update );

int get_tsd_remote_update_brdg_ctrl_load_fctry_img( uint32_t base, uint32_t *load_fctry_img );
int set_tsd_remote_update_brdg_ctrl_load_fctry_img( uint32_t base, uint32_t load_fctry_img );

int get_tsd_remote_update_brdg_ctrl_veryify_header( uint32_t base, uint32_t *veryify_header );
int set_tsd_remote_update_brdg_ctrl_veryify_header( uint32_t base, uint32_t veryify_header );

int get_tsd_remote_update_brdg_data_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_data_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_l26_time_tick_direction_l26_time_tick( uint32_t base, uint32_t *l26_time_tick );
int set_tsd_remote_update_brdg_l26_time_tick_direction_l26_time_tick( uint32_t base, uint32_t l26_time_tick );

int get_tsd_remote_update_brdg_status_percent_done( uint32_t base, uint32_t *percent_done );

int get_tsd_remote_update_brdg_status_flash_copy( uint32_t base, uint32_t *flash_copy );

int get_tsd_remote_update_brdg_status_error_code( uint32_t base, uint32_t *error_code );

int get_tsd_remote_update_brdg_status_verify( uint32_t base, uint32_t *verify );

int get_tsd_remote_update_brdg_status_factory( uint32_t base, uint32_t *factory );

int get_tsd_remote_update_brdg_status_busy( uint32_t base, uint32_t *busy );

int get_tsd_remote_update_brdg_status_erasing( uint32_t base, uint32_t *erasing );

int get_tsd_remote_update_brdg_front_panel_brd_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_front_panel_brd_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_front_panel_brd_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_front_panel_brd_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_front_panel_brd_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_front_panel_brd_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_switch_brd_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_switch_brd_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_switch_brd_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_switch_brd_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_switch_brd_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_switch_brd_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_fpga_brd_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_fpga_brd_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_fpga_brd_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_fpga_brd_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_fpga_brd_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_fpga_brd_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_power_brd_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_power_brd_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_power_brd_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_power_brd_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_power_brd_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_power_brd_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_serial_num_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_serial_num_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_serial_num_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_serial_num_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_serial_num_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_serial_num_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_model_num_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_model_num_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_model_num_revision( uint32_t base, uint32_t *revision );
int set_tsd_remote_update_brdg_model_num_revision( uint32_t base, uint32_t revision );

int get_tsd_remote_update_brdg_model_num_brd_number( uint32_t base, uint32_t *brd_number );
int set_tsd_remote_update_brdg_model_num_brd_number( uint32_t base, uint32_t brd_number );

int get_tsd_remote_update_brdg_scratch_1_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_scratch_1_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_scratch_2_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_scratch_2_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_scratch_3_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_scratch_3_reg( uint32_t base, uint32_t reg );

int get_tsd_remote_update_brdg_scratch_4_reg( uint32_t base, uint32_t *reg );
int set_tsd_remote_update_brdg_scratch_4_reg( uint32_t base, uint32_t reg );




#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif /* __TSD_REMOTE_UPDATE_BRDG_H__ */
