/*
 * mitr_fpga_api.h
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */

#ifndef MITR_FPGA_API_H_
#define MITR_FPGA_API_H_


#include "main_types.h"     /* For vtss_init_data_t */
#include "vtss_module_id.h" /* For vtss_module_id_t */
#include "mitr_types.h"



#define HIDE_IRIG_INPUTS	1

/**
 * \brief Module initialization function
 */
vtss_rc mitr_fpga_init(vtss_init_data_t *data);

void Get_PortState(u32 *link, u32 *activity, u32 *speed);

int Set_leds_and_discretes(u32 link, u32 activity, u32 speed);

int GetSystemTime(time_t *t);

int SetSystemTime(time_t t);


#endif /* MITR_FPGA_API_H_ */
