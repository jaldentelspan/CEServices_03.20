/*
 * tsd_pll_reconfig.h
 *
 *  Created on: Jan 5, 2021
 *      Author: eric
 */

#ifndef TSD_PLL_RECONFIG_H_
#define TSD_PLL_RECONFIG_H_

/* #include <stdbool.h> */
/* #include <stdint.h> */
#include "main.h"
#include "tsd_pll_reconfig_regs.h"

#define TSD_PLL_RECONFIG_SUCCESS	0
#define TSD_PLL_RECONFIG_BUSY_FAIL	1
#define TSD_PLL_RECONFIG_CALC_FAIL	2
#define TSD_PLL_RECONFIG_OTHER_FAIL	3

typedef struct ReconfigPLLInfo_t {
	int nCounter;
	int mCounter;
	int cCounter;
	int kCounter;
	int bandwithVal;
	int chargePumpVal;
	int referencePllFreq;
}ReconfigPLLInfo;

/*
 * Reconfigures the PLL to the newBitrateHz value.
 * base: the Base address of the PLL reconfiguration component
 * referenceFreqHz: The PLL reference (input) frequency.
 * pllInfo: (optional) stores the values set into the PLL Reconfig.
 *          NULL may be sent into the function if not required.
 *          this information is required in order to back calculate the current bitrate.
 * NOTE: Prior to calling this function, ensure that the pll reconfiguration block
 * is NOT busy (e.g. pll_reconfiguration_block_busy() returns false).
 * It's also a good idea to also ensure that reconfiguration_block_busy() returns false
 * after calling this function and before the pll is used by another component.
 * returns 0 on success.
 */
int reconfigure_pll_bitrate(uint32_t base, int referenceFreqHz, int newBitrateHz, ReconfigPLLInfo *pllInfo);

/*
 * returns a value indicating if the reconfiguration block located at
 * base address 'base' is currently busy.
 * return values:
 * false: the reconfiguration block is not busy.
 * true: the reconfiguration block is busy and cannot accept any commands.
 */
bool pll_reconfiguration_block_busy(uint32_t base);

/*
 * Calculates that bitrate that the PLL should be producing given the input pllInfo.
 * returns the calculated bitrate.
 */
int pll_reconfiguration_get_frequency(ReconfigPLLInfo pllInfo);

#endif /* TSD_PLL_RECONFIG_H_ */
