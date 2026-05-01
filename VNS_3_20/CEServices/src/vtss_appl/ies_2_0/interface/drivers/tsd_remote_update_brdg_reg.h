/*
 *
 *  tsd_remote_update_brdg_reg.h
 *
 * Created on: 2020-01-02 13:45
 *     Author: jalden
 *
 */


#ifndef __TSD_REMOTE_UPDATE_BRDG_REG_H__
#define __TSD_REMOTE_UPDATE_BRDG_REG_H__
#include <io.h>



///////////////////////////////////////////////////////////////////////////
//// Regesters
///////////////////////////////////////////////////////////////////////////

#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG                      (0x00)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG                      (0x01)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_REG                             (0x02)
#define TSD_REMOTE_UPDATE_BRDG_DATA_REG                             (0x03)
#define TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_REG          (0x04)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_REG                           (0x05)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG                  (0x06)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG                       (0x07)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG                         (0x08)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG                        (0x09)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG                       (0x0A)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG                        (0x0B)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG                        (0x0C)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG                        (0x0D)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG                        (0x0E)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG                        (0x0F)

///////////////////////////////////////////////////////////////////////////
//// Regester Bits\ Masks
///////////////////////////////////////////////////////////////////////////

#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_SECONDS_MASK       (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_SECONDS_SHFT                (0)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_MINUTES_MASK       (0x0000FF00)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_MINUTES_SHFT                (8)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_HOURS_MASK         (0x00FF0000)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_HOURS_SHFT                 (16)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG_MASK           (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG_SHFT                    (0)

#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_DAY_MASK           (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_DAY_SHFT                    (0)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_MONTH_MASK         (0x0000FF00)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_MONTH_SHFT                  (8)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_YEAR_MASK          (0xFFFF0000)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_YEAR_SHFT                  (16)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG_MASK           (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG_SHFT                    (0)

#define TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_MASK    (0x08000000)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_COPY_APP_2_FACTORY_SHFT            (27)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_MASK        (0x10000000)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_ERASE_APP_IMG_SHFT                (28)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_MASK         (0x20000000)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_START_UPDATE_SHFT                 (29)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_MASK       (0x40000000)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_LOAD_FCTRY_IMG_SHFT               (30)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_MASK       (0x80000000)
#define TSD_REMOTE_UPDATE_BRDG_CTRL_VERYIFY_HEADER_SHFT               (31)

#define TSD_REMOTE_UPDATE_BRDG_DATA_REG_MASK                  (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_DATA_REG_SHFT                           (0)

#define TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_MASK    (0x00000001)
#define TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_L26_TIME_TICK_SHFT             (0)

#define TSD_REMOTE_UPDATE_BRDG_STATUS_PERCENT_DONE_MASK       (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_PERCENT_DONE_SHFT                (0)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_FLASH_COPY_MASK         (0x00800000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_FLASH_COPY_SHFT                 (23)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_ERROR_CODE_MASK         (0x0F000000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_ERROR_CODE_SHFT                 (24)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_VERIFY_MASK             (0x10000000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_VERIFY_SHFT                     (28)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_FACTORY_MASK            (0x20000000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_FACTORY_SHFT                    (29)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_BUSY_MASK               (0x40000000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_BUSY_SHFT                       (30)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_ERASING_MASK            (0x80000000)
#define TSD_REMOTE_UPDATE_BRDG_STATUS_ERASING_SHFT                    (31)

#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_MASK       (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG_SHFT                (0)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_MASK    (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REVISION_SHFT             (0)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_MASK    (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_BRD_NUMBER_SHFT             (8)

#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_MASK            (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG_SHFT                     (0)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_MASK       (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REVISION_SHFT                (0)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_MASK     (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_BRD_NUMBER_SHFT              (8)

#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_MASK              (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG_SHFT                       (0)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_MASK         (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REVISION_SHFT                  (0)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_MASK       (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_BRD_NUMBER_SHFT                (8)

#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG_SHFT                      (0)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_MASK        (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REVISION_SHFT                 (0)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_MASK      (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_POWER_BRD_BRD_NUMBER_SHFT               (8)

#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_MASK            (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG_SHFT                     (0)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_MASK       (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REVISION_SHFT                (0)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_MASK     (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_BRD_NUMBER_SHFT              (8)

#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG_SHFT                      (0)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_MASK        (0x000000FF)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REVISION_SHFT                 (0)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_MASK      (0xFFFFFF00)
#define TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_BRD_NUMBER_SHFT               (8)

#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG_SHFT                      (0)

#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG_SHFT                      (0)

#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG_SHFT                      (0)

#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_MASK             (0xFFFFFFFF)
#define TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG_SHFT                      (0)


///////////////////////////////////////////////////////////////////////////
//// Regester Access
///////////////////////////////////////////////////////////////////////////

#define TSD_REMOTE_UPDATE_BRDG_IORD_DATE_CODE_L(base)      \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_DATE_CODE_L_REG)     


#define TSD_REMOTE_UPDATE_BRDG_IORD_DATE_CODE_H(base)      \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_DATE_CODE_H_REG)     


#define TSD_REMOTE_UPDATE_BRDG_IORD_CTRL(base)             \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_CTRL_REG)            
#define TSD_REMOTE_UPDATE_BRDG_IOWR_CTRL(base, val)        \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_CTRL_REG, val)       


#define TSD_REMOTE_UPDATE_BRDG_IORD_DATA(base)             \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_DATA_REG)            
#define TSD_REMOTE_UPDATE_BRDG_IOWR_DATA(base, val)        \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_DATA_REG, val)       


#define TSD_REMOTE_UPDATE_BRDG_IORD_L26_TIME_TICK_DIRECTION(base) \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_REG)
#define TSD_REMOTE_UPDATE_BRDG_IOWR_L26_TIME_TICK_DIRECTION(base, val) \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_L26_TIME_TICK_DIRECTION_REG, val)


#define TSD_REMOTE_UPDATE_BRDG_IORD_STATUS(base)           \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_STATUS_REG)          


#define TSD_REMOTE_UPDATE_BRDG_IORD_FRONT_PANEL_BRD(base)  \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG) 
#define TSD_REMOTE_UPDATE_BRDG_IOWR_FRONT_PANEL_BRD(base, val) \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_FRONT_PANEL_BRD_REG, val)


#define TSD_REMOTE_UPDATE_BRDG_IORD_SWITCH_BRD(base)       \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG)      
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SWITCH_BRD(base, val)  \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SWITCH_BRD_REG, val) 


#define TSD_REMOTE_UPDATE_BRDG_IORD_FPGA_BRD(base)         \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG)        
#define TSD_REMOTE_UPDATE_BRDG_IOWR_FPGA_BRD(base, val)    \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_FPGA_BRD_REG, val)   


#define TSD_REMOTE_UPDATE_BRDG_IORD_POWER_BRD(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_POWER_BRD(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_POWER_BRD_REG, val)  


#define TSD_REMOTE_UPDATE_BRDG_IORD_SERIAL_NUM(base)       \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG)      
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SERIAL_NUM(base, val)  \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SERIAL_NUM_REG, val) 


#define TSD_REMOTE_UPDATE_BRDG_IORD_MODEL_NUM(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_MODEL_NUM(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_MODEL_NUM_REG, val)  


#define TSD_REMOTE_UPDATE_BRDG_IORD_SCRATCH_1(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SCRATCH_1(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_1_REG, val)  


#define TSD_REMOTE_UPDATE_BRDG_IORD_SCRATCH_2(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SCRATCH_2(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_2_REG, val)  


#define TSD_REMOTE_UPDATE_BRDG_IORD_SCRATCH_3(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SCRATCH_3(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_3_REG, val)  


#define TSD_REMOTE_UPDATE_BRDG_IORD_SCRATCH_4(base)        \
                      IORD(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG)       
#define TSD_REMOTE_UPDATE_BRDG_IOWR_SCRATCH_4(base, val)   \
                      IOWR(base, TSD_REMOTE_UPDATE_BRDG_SCRATCH_4_REG, val)  



#endif /* __TSD_REMOTE_UPDATE_BRDG_REG_H__ */
