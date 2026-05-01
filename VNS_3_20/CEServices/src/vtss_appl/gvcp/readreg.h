/*
 * readreg.h
 *
 *  Created on: Aug 25, 2014
 *      Author: jalden
 */

#ifndef READREG_H_
#define READREG_H_
#include "gvcp_types.h"

#define CCP_REG	0x0A00
#define MAC_TABLE_REG	0xF000

typedef struct gvcp_register
{

	unsigned int name;
	unsigned int value;
	const unsigned short read;
	const unsigned short write;

} gvcp_register_t ;

static gvcp_register_t gvcp_reg[4] = {	{ 0x0A00, 0x00000003, 0x01, 0x01},
								{ 0x0B000, 0x00000004, 0x01, 0x01},
								{ 0x0C000, 0x00000005, 0x01, 0x01},
								{ 0x0F000, 0x00000005, 0x01, 0x00}};

int Gvcp_Read_Reg_Batch(linked_list_t *resp_list,linked_list_t *cmd_list);
int Gvcp_Read_Reg(unsigned int reg,unsigned int *val);
int Gvcp_Write_Reg(unsigned int reg, unsigned int val);



#endif /* READREG_H_ */
