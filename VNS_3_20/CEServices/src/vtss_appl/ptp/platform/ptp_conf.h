/*
 * ptp_conf.h
 *
 *  Created on: Mar 7, 2017
 *      Author: jalden
 */

#ifndef VTSS_APPL_PTP_PLATFORM_PTP_CONF_H_
#define VTSS_APPL_PTP_PLATFORM_PTP_CONF_H_
//#include "ptp_api.h"
//#include "ptp.h"
////#include "vtss_ptp_types.h"
//
//#include "vtss_ptp_synce_api.h"
//
///**
// * \brief PTP Clock Config Data Set structure
// */
//typedef struct ptp_instance_config_t {
//    ptp_init_clock_ds_t         clock_init;
//    ptp_set_clock_ds_t          clock_ds;
//    ptp_clock_timeproperties_ds_t time_prop;
//    ptp_set_port_ds_t               port_config [PTP_CLOCK_PORTS];
//    vtss_ptp_default_filter_config_t default_of; /* default offset filter config */
//    vtss_ptp_default_servo_config_t default_se; /* default servo config */
//    vtss_ptp_default_delay_filter_config_t default_df; /* default delay filter  config*/
//    vtss_ptp_unicast_slave_config_t unicast_slave[MAX_UNICAST_MASTERS_PR_SLAVE]; /* Unicast slave config, i.e. requested master(s) */
//    ptp_clock_slave_cfg_t       slave_cfg;
//} ptp_instance_config_t;
//
//
//typedef struct ptp_config_t {
//    i8 version;             /* configuration version, to be changed if this structure is changed */
//    ptp_instance_config_t conf [PTP_CLOCK_INSTANCES];
//    init_synce_t init_synce;  /* syncE VCXO calibration value */
//    vtss_ptp_ext_clock_mode_t init_ext_clock_mode; /* luton26/Jaguar/Serval(Synce) external clock mode */
//#if defined (VTSS_ARCH_SERVAL)
//    vtss_ptp_rs422_conf_t init_ext_clock_mode_rs422; /* Serval(RS422) external clock mode */
//#endif // (VTSS_ARCH_SERVAL)
//} ptp_config_t;
//
//static ptp_config_t config_data ;
//
///**
// *
// */
//ptp_config_t* get_ptp_master_config(void);
//
///**
// *
// */
//BOOL set_ptp_master_config(const ptp_config_t conf_dat);
//
//void ptp_conf_save(void);

#endif /* VTSS_APPL_PTP_PLATFORM_PTP_CONF_H_ */
