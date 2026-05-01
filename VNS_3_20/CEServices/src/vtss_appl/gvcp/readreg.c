/*
 * readreg.c
 *
 *  Created on: Aug 25, 2014
 *      Author: jalden
 */

#include <network.h>
#include "readreg.h"
#include "gvcp_api.h"

int Read_Reg(unsigned int reg,unsigned int *val);

int Gvcp_Read_Reg_Batch(linked_list_t *resp_list,linked_list_t *cmd_list)
{
//***************************************************************************************************
//*		Function: 	Retrieves responses from a list of register address to be read. should return 	*
//*					number of successful  reads.  													*
//*					Currently, it can only handle a list of one and it puts the responses in the 	*
//*					same list.																		*
//***************************************************************************************************
	//
	cmd_list->conductor = cmd_list->head;
	unsigned int *swapped_ptr = cmd_list->conductor->data;
	unsigned int swapped = *swapped_ptr;
	swapped =	htonl(swapped);
	unsigned int response = 0;

	if(resp_list == cmd_list)
	{
		Read_Reg(swapped, &response);
		response = htonl(response);
		memcpy(resp_list->conductor->data,&response, sizeof(unsigned int));
		resp_list->conductor->data_size = sizeof(unsigned int);
		return 1;
	}
	return 0;
}
int Read_Reg(unsigned int reg,unsigned int *val)
{
	// returning a negative number is a failure // 0 for success // positive indicates number of additional elements attached
	unsigned short i;


gvcpT_D("Register to read is 0x%08X ",reg);

// build MAC table when MAC_TABLE_REG is read
	if((reg & MAC_TABLE_REG) == MAC_TABLE_REG)
	{
		if(reg == MAC_TABLE_REG)
		{
gvcpT_D("Clearing MAC Table \n");
			clear_list(&MAC_TABLE);
gvcpT_D("executing: *val = build_mac_table(&MAC_TABLE) \n");
			*val = build_mac_table(&MAC_TABLE);
			print_mac_table();
			return 0;
		}
		//check to see if register is within range
		unsigned int reg_query = reg & 0x0FFF;// Mask out top bit
		if((MAC_TABLE.list_size << 1) < reg_query)
		{
gvcpT_D("Failure: Call is out of bounds..\n ");
			*val = 0x0000;
			return -1;
		}
		if(MAC_TABLE.head == NULL)
		{
			gvcpT_D("MAC_TABLE is empty!) ");
								*val = 0x0000;
								return -1;
		}
		// loop through list to find a response

//		int cnt = 0;
		MAC_TABLE.conductor = MAC_TABLE.head;
		gvcpreg_mac_table_entry_t *gvcpreg_mac_entry;
gvcpT_D("Searching for registry....\n");

		while(MAC_TABLE.conductor != NULL)
		{

gvcpT_D("1\n");
			gvcpreg_mac_entry = MAC_TABLE.conductor->data;
gvcpT_D("\t%X",reg);
gvcpT_D("\t%X",gvcpreg_mac_entry->reg_addr_high);
gvcpT_D("\t%X",gvcpreg_mac_entry->reg_addr_low);

			if(reg == gvcpreg_mac_entry->reg_addr_high)
			{
gvcpT_D("Match found: upper protion of MAC addr & port\n");
				*val = gvcpreg_mac_entry->mactbl_addr_high;
				return 0;
			}
gvcpT_D("2");
			if(reg == gvcpreg_mac_entry->reg_addr_low)
			{
gvcpT_D("Match found: lower protion of MAC addr");
				*val = gvcpreg_mac_entry->mactbl_addr_low;
				return 0;
			}
gvcpT_D("3\n");
			MAC_TABLE.conductor = MAC_TABLE.conductor->next;
		}

gvcpT_D("Error:Broke out of loop: register was not found");
		return -1;


	}
	else
	{
//gvcpT_D("executing: build_mac_table(&MAC_TABLE) ");
//				build_mac_table(&MAC_TABLE);
		for(i = 0; i < sizeof(gvcp_reg); i++)
		{
			if((gvcp_reg[i].name == reg))
			{
gvcpT_D("executing: *val = gvcp_reg[i].value; ");
				*val = gvcp_reg[i].value;
				return (0);
			}
		}
	}




	return (-1);
}
int Write_Reg(unsigned int reg, unsigned int val)
{
	unsigned short i;
	for(i = 0; i < sizeof(gvcp_reg); i++)
	{
		if((gvcp_reg[i].name == reg) && (gvcp_reg[i].write))
		{
			gvcp_reg[i].value = val;
			return (0);
		}
	}

	return (-1);
}
