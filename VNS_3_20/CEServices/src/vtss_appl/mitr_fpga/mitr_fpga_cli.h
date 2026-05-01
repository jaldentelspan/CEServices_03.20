/*
 * mitr_fpga_cli.h
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */

#ifndef MITR_FPGA_CLI_H_
#define MITR_FPGA_CLI_H_

/**
 *  \file mitr_fpga_cli.h
 *  \brief This file defines the interface to the MITR FPGA CLI Commands.
 */

/**
 *  \brief Initialization function.
 *
 *  Call once, preferably from the INIT_CMD_INIT section of
 *  the module's _init() function.
 */
void mitr_fpga_cli_init(void);


#endif /* MITR_FPGA_CLI_H_ */
