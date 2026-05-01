/*
 * vns_fpga_cli.h
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */

#ifndef VNS_FPGA_CLI_H_
#define VNS_FPGA_CLI_H_

/**
 *  \file vns_fpga_cli.h
 *  \brief This file defines the interface to the VNS FPGA CLI Commands.
 */

/**
 *  \brief Initialization function.
 *
 *  Call once, preferably from the INIT_CMD_INIT section of
 *  the module's _init() function.
 */
void vns_fpga_cli_init(void);


#endif /* VNS_FPGA_CLI_H_ */
