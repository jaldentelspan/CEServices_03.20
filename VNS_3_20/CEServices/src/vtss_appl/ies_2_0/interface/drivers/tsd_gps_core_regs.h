/*                                                                                                                                    
 * tsd_gps_registers.h                                                                                                                
 *                                                                                                                                    
 *  Created on: Sep 5, 2017                                                                                                           
 *      Author: eric                                                                                                                  
 */                                                                                                                                   
                                                                                                                                      
#ifndef TSD_GPS_REGISTERS_H_                                                                                                          
#define TSD_GPS_REGISTERS_H_                                                                                                          
                                                                                                                                      
#include <io.h>                                                                                                                       
                                                                                                                                      
/**************************************                                                                                               
 * GPS DEFINES                                                                                                                        
 **************************************/                                                                                              
#define TSD_GPS_CONTROL_ENABLE          (0x01)                                                                                        
#define TSD_GPS_CONTROL_PWR_3P3V        (0x02)                                                                                        
#define TSD_GPS_CONTROL_PWR_5P0V        (0x04)                                                                                        
#define TSD_GPS_CONTROL_ANT_3P3V        (0x08)                                                                                        
#define TSD_GPS_CONTROL_ANT_5P0V        (0x10)                                                                                        
#define TSD_GPS_CONTROL_PWR_FLAG_MASK   (0x0000001E)                                                                                  
#define TSD_GPS_CONTROL_PWR_FLAG_SHFT   (0)                                                                                           
#define TSD_GPS_CONTROL_BAUD_RATE_MASK  (0x3FF00000)                                                                                  
#define TSD_GPS_CONTROL_BAUD_RATE_SHFT  (20)                                                                                          
#define TSD_GPS_CONTROL_RESET           (0x80000000)                                                                                  
                                                                                                                                      
#define TSD_GPS_STATUS_INTERNAL_ERR     (0x01)                                                                                        
#define TSD_GPS_STATUS_SETUP_ERR        (0x02)                                                                                        
#define TSD_GPS_STATUS_NO_SIGNAL_ERR    (0x04)                                                                                        
#define TSD_GPS_STATUS_BAD_SIGNAL_ERR   (0x08)                                                                                        
#define TSD_GPS_STATUS_NOT_LOCKED_ERR   (0x10)                                                                                        
#define TSD_GPS_STATUS_NOT_ENABLED_ERR  (0x20) /* gps core not running... enable bit == 0 */                                          
#define TSD_GPS_STATUS_WINDOW_ERR       (0x40)                                                                                        
                                                                                                                                      
#define    TSD_GPS_LEAP_SECOND_REG_MASK (0x000000FF) /* Current Leap seconds */                                                       
#define    TSD_GPS_LEAP_SECOND_REG_SHFT (0)                                                                                           
                                                                                                                                      
#define TSD_GPS_POLY_LEAP_SEC_MASK      (0x000000FF) /* Leap seconds read from GPS */                                                 
#define TSD_GPS_POLY_FOUND_LEAP_SEC_MSG (0x100)                                                                                       
#define TSD_GPS_POLY_LEAP_SEC_MSG_DONE  (0x200)                                                                                       
                                                                                                                                      
#define TSD_GPS_TIME_L_BCD_MSEC_MASK    (0x000000FF)                                                                                  
#define TSD_GPS_TIME_L_BCD_MSEC_SHFT    (0)                                                                                           
#define TSD_GPS_TIME_L_BCD_SECONDS_MASK (0x0000FF00)                                                                                  
#define TSD_GPS_TIME_L_BCD_SECONDS_SHFT (8)                                                                                           
#define TSD_GPS_TIME_L_BCD_MINUTES_MASK (0x00FF0000)                                                                                  
#define TSD_GPS_TIME_L_BCD_MINUTES_SHFT (16)                                                                                          
#define TSD_GPS_TIME_L_BCD_HOURS_MASK   (0xFF000000)                                                                                  
#define TSD_GPS_TIME_L_BCD_HOURS_SHFT   (24)                                                                                          
                                                                                                                                      
#define TSD_GPS_TIME_H_BCD_DAYS_MASK    (0x00000FFF)                                                                                  
#define TSD_GPS_TIME_H_BCD_DAYS_SHFT    (0)                                                                                           
#define TSD_GPS_TIME_H_BCD_YEARS_MASK   (0x00FF0000)                                                                                  
#define TSD_GPS_TIME_H_BCD_YEARS_SHFT   (16)                                                                                          
                                                                                                                                      
#define TSD_GPS_LOST_LOCK_CNTR_MASK     (0x7FFFFFFF)                                                                                  
#define TSD_GPS_LOST_LOCK_CNTR_RESET    (0x80000000)                                                                                  
                                                                                                                                      
/**************************************                                                                                               
 * GPS REGISTERS                                                                                                                      
 **************************************/                                                                                              
#define TSD_GPS_VERSION_REG             (0x00)                                                                                        
#define TSD_GPS_CONTROL_REG             (0x01)                                                                                        
#define TSD_GPS_STATUS_REG              (0x02)                                                                                        
#define TSD_GPS_LEAP_SEC_REG            (0x03)                                                                                        
#define TSD_GPS_POLY_MSG_REG            (0x04)                                                                                        
#define TSD_GPS_TIME_L_REG              (0x05)                                                                                        
#define TSD_GPS_TIME_H_REG              (0x06)                                                                                        
#define TSD_GPS_PPS_WINDOW_MIN_REG      (0x07)                                                                                        
#define TSD_GPS_PPS_WINDOW_MAX_REG      (0x08)                                                                                        
#define TSD_GPS_PPS_MTICK_CNT_REG       (0x09)                                                                                        
#define TSD_GPS_LOST_LOCK_CNTR_REG      (0x0A)                                                                                        
                                                                                                                                      
/**************************************                                                                                               
 * GPS REGISTER ACCESS                                                                                                                
 **************************************/                                                                                              
                                                                                                                                      
#define TSD_GPS_IORD_VERSION(base)              IORD(base, TSD_GPS_VERSION_REG)                                                       
                                                                                                                                      
#define TSD_GPS_IORD_CONTROL(base)              IORD(base, TSD_GPS_CONTROL_REG)                                                       
#define TSD_GPS_IOWR_CONTROL(base, val)         IOWR(base, TSD_GPS_CONTROL_REG, val)                                                  
                                                                                                                                      
#define TSD_GPS_IORD_STATUS(base)               IORD(base, TSD_GPS_STATUS_REG)                                                        
                                                                                                                                      
#define TSD_GPS_IORD_LEAP_SEC(base)             IORD(base, TSD_GPS_LEAP_SEC_REG)                                                      
#define TSD_GPS_IOWR_LEAP_SEC(base, val)        IOWR(base, TSD_GPS_LEAP_SEC_REG, val)                                                 
                                                                                                                                      
#define TSD_GPS_IORD_POLY_MSG(base)             IORD(base, TSD_GPS_POLY_MSG_REG)                                                      
                                                                                                                                      
#define TSD_GPS_IORD_TIME_L(base)               IORD(base, TSD_GPS_TIME_L_REG)                                                        
                                                                                                                                      
#define TSD_GPS_IORD_TIME_H(base)               IORD(base, TSD_GPS_TIME_H_REG)                                                        
                                                                                                                                      
#define TSD_GPS_IORD_PPS_WINDOW_MIN(base)       IORD(base, TSD_GPS_PPS_WINDOW_MIN_REG)                                                
#define TSD_GPS_IOWR_PPS_WINDOW_MIN(base, val)  IOWR(base, TSD_GPS_PPS_WINDOW_MIN_REG, val)                                           
                                                                                                                                      
#define TSD_GPS_IORD_PPS_WINDOW_MAX(base)       IORD(base, TSD_GPS_PPS_WINDOW_MAX_REG)                                                
#define TSD_GPS_IOWR_PPS_WINDOW_MAX(base, val)  IOWR(base, TSD_GPS_PPS_WINDOW_MAX_REG, val)                                           
                                                                                                                                      
#define TSD_GPS_IORD_PPS_MTICK_CNT(base)        IORD(base, TSD_GPS_PPS_MTICK_CNT_REG)                                                 
                                                                                                                                      
#define TSD_GPS_IORD_LOST_LOCK_CNTR(base)       IORD(base, TSD_GPS_LOST_LOCK_CNTR_REG)                                                
#define TSD_GPS_IOWR_LOST_LOCK_CNTR(base, val)  IOWR(base, TSD_GPS_LOST_LOCK_CNTR_REG, val)                                           
                                                                                                                                      
#endif /* TSD_GPS_REGISTERS_H_ */                                                                                                     
                                                                                                                                      
