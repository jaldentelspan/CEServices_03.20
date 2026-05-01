/*
 * vns_fpga_ptp.h
 *
 *  Created on: Aug 6, 2013
 *      Author: eric
 */

#ifndef VNS_FPGA_PTP_H_
#define VNS_FPGA_PTP_H_

int status_ptp(ptp_clock_default_ds_t *clock_bs,
						vtss_ptp_ext_clock_mode_t *mode,
						vtss_timestamp_t *t,
						ptp_clock_slave_ds_t *clock_slave_bs);


#endif /* VNS_FPGA_PTP_H_ */
