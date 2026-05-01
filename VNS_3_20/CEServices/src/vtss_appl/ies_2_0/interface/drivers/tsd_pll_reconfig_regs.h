/*
 * tsd_pll_reconfig.h
 *
 *  Created on: Jan 5, 2021
 *      Author: eric
 */

#ifndef TSD_PLL_RECONFIG_REGS_H_
#define TSD_PLL_RECONFIG_REGS_H_
#include "io.h"

/*
 * COMMENT THIS OUT FOR NEW COMMON IP.
 * IF USING THE OLD, INDIRECT METHOD THROUGH REGISTER BRIDGE MAKE THIS DEFINE
 */
#define DO_IT_THE_OLD_WAY

/*********************************************************
 * PLL RECONFIG REGISTERS
 *********************************************************/

#define TSD_PLL_RECONFIG_MODE_REG			(0x0)
#define TSD_PLL_RECONFIG_STATUS_REG			(0x1)
#define TSD_PLL_RECONFIG_START_REG			(0x2)
#define TSD_PLL_RECONFIG_N_COUNTER_REG		(0x3)
#define TSD_PLL_RECONFIG_M_COUNTER_REG		(0x4)
#define TSD_PLL_RECONFIG_C_COUNTER_REG		(0x5)
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_REG	(0x6)
#define TSD_PLL_RECONFIG_K_COUNTER_REG		(0x7) /* M COUNTER FRACTIONAL VALUE */
#define TSD_PLL_RECONFIG_BANDWIDTH_REG		(0x8)
#define TSD_PLL_RECONFIG_CHARGE_PUMP_REG	(0x9)
#define TSD_PLL_RECONFIG_VCO_POST_DIV_REG	(0x1C)

/*********************************************************
 * PLL RECONFIG MODE DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_MODE_WAITREQUEST	0x0
#define TSD_PLL_RECONFIG_MODE_POLLING		0x1

/*********************************************************
 * PLL RECONFIG STATUS DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_STATUS_BUSY		0x0
#define TSD_PLL_RECONFIG_STATUS_READY		0x1

/*********************************************************
 * PLL RECONFIG STATUS DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_START_BIT			0x1

/*********************************************************
 * PLL RECONFIG N COUNTER DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_N_COUNTER_LOW_COUNT_MASK	(0x000000FF)
#define TSD_PLL_RECONFIG_N_COUNTER_LOW_COUNT_SHFT	(0)
#define TSD_PLL_RECONFIG_N_COUNTER_HIGH_COUNT_MASK	(0x0000FF00)
#define TSD_PLL_RECONFIG_N_COUNTER_HIGH_COUNT_SHFT	(8)
#define TSD_PLL_RECONFIG_N_COUNTER_BYPASS_ENABLE	(0x00010000)
#define TSD_PLL_RECONFIG_N_COUNTER_ODD_DIVISION		(0x00020000)

/*********************************************************
 * PLL RECONFIG M COUNTER DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_M_COUNTER_LOW_COUNT_MASK	(0x000000FF)
#define TSD_PLL_RECONFIG_M_COUNTER_LOW_COUNT_SHFT	(0)
#define TSD_PLL_RECONFIG_M_COUNTER_HIGH_COUNT_MASK	(0x0000FF00)
#define TSD_PLL_RECONFIG_M_COUNTER_HIGH_COUNT_SHFT	(8)
#define TSD_PLL_RECONFIG_M_COUNTER_BYPASS_ENABLE	(0x00010000)
#define TSD_PLL_RECONFIG_M_COUNTER_ODD_DIVISION		(0x00020000)

/*********************************************************
 * PLL RECONFIG C COUNTER DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_C_COUNTER_LOW_COUNT_MASK	(0x000000FF)
#define TSD_PLL_RECONFIG_C_COUNTER_LOW_COUNT_SHFT	(0)
#define TSD_PLL_RECONFIG_C_COUNTER_HIGH_COUNT_MASK	(0x0000FF00)
#define TSD_PLL_RECONFIG_C_COUNTER_HIGH_COUNT_SHFT	(8)
#define TSD_PLL_RECONFIG_C_COUNTER_BYPASS_ENABLE	(0x00010000)
#define TSD_PLL_RECONFIG_C_COUNTER_ODD_DIVISION		(0x00020000)
#define TSD_PLL_RECONFIG_C_COUNTER_MASK				(0x007C0000)
#define TSD_PLL_RECONFIG_C_COUNTER_SHFT				(18)

/*********************************************************
 * PLL RECONFIG DYNAMIC PHASE SHIFT DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_NUM_SHFTS_MASK	(0x0000FFFF)
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_NUM_SHFTS_SHFT	(0)
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_CNT_SEL_MASK	(0x00FF0000)
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_CNT_SEL_SHFT	(16)
#define TSD_PLL_RECONFIG_DYN_PHASE_SHFT_POS_PHASE_SHFT	(0x01000000)

/*********************************************************
 * PLL RECONFIG BANDWIDTH SETTING DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_BANDWIDTH_SETTING_MASK			(0x0000000F)

/*********************************************************
 * PLL RECONFIG CHARGE PUMP SETTING DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_CHARGE_PUMP_SETTING_MASK		(0x00000007)

/*********************************************************
 * PLL RECONFIG POST DIVIDER SETTING DEFINITIONS
 *********************************************************/
#define TSD_PLL_RECONFIG_VCO_POST_DIV_2					(0x00000000)
#define TSD_PLL_RECONFIG_VCO_POST_DIV_1					(0x00000001)

/*********************************************************
 * PLL RECONFIG IO CALLS
 *********************************************************/
#ifdef DO_IT_THE_OLD_WAY
void     write_reconfig_address(uint32_t base, uint32_t reconfigReg, uint32_t val);
uint32_t read_reconfig_address(uint32_t base, uint32_t reconfigReg);
#define TSD_PLL_RECONFIG_IORD_MODE(base)  			read_reconfig_address(base, TSD_PLL_RECONFIG_MODE_REG)
#define TSD_PLL_RECONFIG_IOWR_MODE(base, val)  		write_reconfig_address(base, TSD_PLL_RECONFIG_MODE_REG, (val & 0x01))

#define TSD_PLL_RECONFIG_IORD_STATUS(base)  		read_reconfig_address(base, TSD_PLL_RECONFIG_STATUS_REG)

#define TSD_PLL_RECONFIG_IOWR_START(base)  			write_reconfig_address(base, TSD_PLL_RECONFIG_START_REG, 1)

#define TSD_PLL_RECONFIG_IORD_N_COUNTER(base)  		read_reconfig_address(base, TSD_PLL_RECONFIG_N_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_N_COUNTER(base, val)  write_reconfig_address(base, TSD_PLL_RECONFIG_N_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_M_COUNTER(base)  		read_reconfig_address(base, TSD_PLL_RECONFIG_M_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_M_COUNTER(base, val)  write_reconfig_address(base, TSD_PLL_RECONFIG_M_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_C_COUNTER(base)  		read_reconfig_address(base, TSD_PLL_RECONFIG_C_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_C_COUNTER(base, val)  write_reconfig_address(base, TSD_PLL_RECONFIG_C_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IOWR_DYN_PHASE_SHFT(base, val)  write_reconfig_address(base, TSD_PLL_RECONFIG_DYN_PHASE_SHFT_REG, val)

#define TSD_PLL_RECONFIG_IOWR_K_COUNTER(base, val)  write_reconfig_address(base, TSD_PLL_RECONFIG_K_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_BANDWIDTH(base)  		read_reconfig_address(base, TSD_PLL_RECONFIG_BANDWIDTH_REG)
#define TSD_PLL_RECONFIG_IOWR_BANDWIDTH(base, val)	write_reconfig_address(base, TSD_PLL_RECONFIG_BANDWIDTH_REG, (val & TSD_PLL_RECONFIG_BANDWIDTH_SETTING_MASK))

#define TSD_PLL_RECONFIG_IORD_CHARGE_PUMP(base)  	read_reconfig_address(base, TSD_PLL_RECONFIG_CHARGE_PUMP_REG)
#define TSD_PLL_RECONFIG_IOWR_CHARGE_PUMP(base, val) write_reconfig_address(base, TSD_PLL_RECONFIG_CHARGE_PUMP_REG, (val & TSD_PLL_RECONFIG_CHARGE_PUMP_SETTING_MASK))

#define TSD_PLL_RECONFIG_IOWR_VCO_POST_DIV(base, val) write_reconfig_address(base, TSD_PLL_RECONFIG_VCO_POST_DIV_REG, (val & 0x01))


#else // do it the new way

#define TSD_PLL_RECONFIG_IORD_MODE(base)  			IORD(base, TSD_PLL_RECONFIG_MODE_REG)
#define TSD_PLL_RECONFIG_IOWR_MODE(base, val)  		IOWR(base, TSD_PLL_RECONFIG_MODE_REG, (val & 0x01))

#define TSD_PLL_RECONFIG_IORD_STATUS(base)  		IORD(base, TSD_PLL_RECONFIG_STATUS_REG)

#define TSD_PLL_RECONFIG_IOWR_START(base)  			IOWR(base, TSD_PLL_RECONFIG_START_REG, 1)

#define TSD_PLL_RECONFIG_IORD_N_COUNTER(base)  		IORD(base, TSD_PLL_RECONFIG_N_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_N_COUNTER(base, val)  IOWR(base, TSD_PLL_RECONFIG_N_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_M_COUNTER(base)  		IORD(base, TSD_PLL_RECONFIG_M_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_M_COUNTER(base, val)  IOWR(base, TSD_PLL_RECONFIG_M_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_C_COUNTER(base)  		IORD(base, TSD_PLL_RECONFIG_C_COUNTER_REG)
#define TSD_PLL_RECONFIG_IOWR_C_COUNTER(base, val)  IOWR(base, TSD_PLL_RECONFIG_C_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IOWR_DYN_PHASE_SHFT(base, val)  IOWR(base, TSD_PLL_RECONFIG_DYN_PHASE_SHFT_REG, val)

#define TSD_PLL_RECONFIG_IOWR_K_COUNTER(base, val)  IOWR(base, TSD_PLL_RECONFIG_K_COUNTER_REG, val)

#define TSD_PLL_RECONFIG_IORD_BANDWIDTH(base)  		IORD(base, TSD_PLL_RECONFIG_BANDWIDTH_REG)
#define TSD_PLL_RECONFIG_IOWR_BANDWIDTH(base, val)	IOWR(base, TSD_PLL_RECONFIG_BANDWIDTH_REG, (val & TSD_PLL_RECONFIG_BANDWIDTH_SETTING_MASK))

#define TSD_PLL_RECONFIG_IORD_CHARGE_PUMP(base)  	IORD(base, TSD_PLL_RECONFIG_CHARGE_PUMP_REG)
#define TSD_PLL_RECONFIG_IOWR_CHARGE_PUMP(base, val) IOWR(base, TSD_PLL_RECONFIG_CHARGE_PUMP_REG, (val & TSD_PLL_RECONFIG_CHARGE_PUMP_SETTING_MASK))

#define TSD_PLL_RECONFIG_IOWR_VCO_POST_DIV(base, val) IOWR(base, TSD_PLL_RECONFIG_VCO_POST_DIV_REG, (val & 0x01))

#endif // do it the new way

#endif /* TSD_PLL_RECONFIG_REGS_H_ */
