/*
 * tsd_pll_reconfig.c
 *
 *  Created on: Jan 5, 2021
 *      Author: eric
 */

#include <math.h>
#include "tsd_pll_reconfig.h"

#include "vtss_trace_api.h"

#ifdef DO_IT_THE_OLD_WAY
#define INDERECT_CTRL       0
#define INDERECT_WRITE_DATA	2
#define INDERECT_READ_DATA  3

#define INDERECT_CTRL_WRITE     0x100
#define INDERECT_CTRL_READ      0x080
#define INDERECT_CTRL_ADDR_MASK 0x3F
void write_reconfig_address(uint32_t base, uint32_t reconfigReg, uint32_t val){
	IOWR(base, INDERECT_CTRL, 0 );
	IOWR(base, INDERECT_WRITE_DATA, val);
	IOWR(base, INDERECT_CTRL, ((reconfigReg & INDERECT_CTRL_ADDR_MASK) | INDERECT_CTRL_WRITE) );
	IOWR(base, INDERECT_CTRL, 0 );
}

uint32_t read_reconfig_address(uint32_t base, uint32_t reconfigReg){
	uint32_t val;
	IOWR(base, INDERECT_CTRL, 0 );
	IOWR(base, INDERECT_CTRL, ((reconfigReg & INDERECT_CTRL_ADDR_MASK) | INDERECT_CTRL_READ) );
	val = IORD(base, INDERECT_READ_DATA);
	IOWR(base, INDERECT_CTRL, 0 );
	return val;
}

#endif

/* PLL related functions */
/* get value for bandwidth register based on M counter val */
static inline
int pll_get_bandwidth(int M) {
    /* derived from plugging in numbers into Altera's PLL reconf spreadsheet */
    if(M >= 97) {
        return 2;
    } else if(M >= 63) {
        return 3;
    } else if(M >= 36) {
        return 4;
    } else if(M >= 17) {
        return 6;
    } else if(M >= 7) {
        return 7;
    } else {
        return 8;
    }
}

/* get value for charge pump register based on M counter val */
static inline
int pll_get_charge_pump(int M) {
    /* derived from plugging in numbers into Altera's PLL reconf spreadsheet */
    if((M == 4) || (M == 5) || (M == 6)) {
    	return 2;
    } else {
        return 1;
    }
}

static inline
int set_pll_n_counter(int base, uint8_t lowCnt, uint8_t highCnt, int bypass, int oddDivision){
	int reg;

	reg = (((lowCnt << TSD_PLL_RECONFIG_N_COUNTER_LOW_COUNT_SHFT) & TSD_PLL_RECONFIG_N_COUNTER_LOW_COUNT_MASK) |
			((highCnt << TSD_PLL_RECONFIG_N_COUNTER_HIGH_COUNT_SHFT) & TSD_PLL_RECONFIG_N_COUNTER_HIGH_COUNT_MASK) |
			(bypass ? TSD_PLL_RECONFIG_N_COUNTER_BYPASS_ENABLE : 0) |
			(oddDivision ? TSD_PLL_RECONFIG_N_COUNTER_ODD_DIVISION : 0));

	TSD_PLL_RECONFIG_IOWR_N_COUNTER(base, reg);

	return ((lowCnt >= 256) || (highCnt >= 256));
}

static inline
int set_pll_m_counter(int base, uint8_t lowCnt, uint8_t highCnt, int bypass, int oddDivision){
	int reg;

	reg = (((lowCnt << TSD_PLL_RECONFIG_M_COUNTER_LOW_COUNT_SHFT) & TSD_PLL_RECONFIG_M_COUNTER_LOW_COUNT_MASK) |
			((highCnt << TSD_PLL_RECONFIG_M_COUNTER_HIGH_COUNT_SHFT) & TSD_PLL_RECONFIG_M_COUNTER_HIGH_COUNT_MASK) |
			(bypass ? TSD_PLL_RECONFIG_M_COUNTER_BYPASS_ENABLE : 0) |
			(oddDivision ? TSD_PLL_RECONFIG_M_COUNTER_ODD_DIVISION : 0));

	TSD_PLL_RECONFIG_IOWR_M_COUNTER(base, reg);

	return ((lowCnt >= 256) || (highCnt >= 256));
}

static inline
int set_pll_c_counter(int base, uint8_t lowCnt, uint8_t highCnt, int bypass, int oddDivision, uint16_t cCounter){
	int reg;

	reg = (((lowCnt << TSD_PLL_RECONFIG_C_COUNTER_LOW_COUNT_SHFT) & TSD_PLL_RECONFIG_C_COUNTER_LOW_COUNT_MASK) |
			((highCnt << TSD_PLL_RECONFIG_C_COUNTER_HIGH_COUNT_SHFT) & TSD_PLL_RECONFIG_C_COUNTER_HIGH_COUNT_MASK) |
			(bypass ? TSD_PLL_RECONFIG_C_COUNTER_BYPASS_ENABLE : 0) |
			(oddDivision ? TSD_PLL_RECONFIG_C_COUNTER_ODD_DIVISION : 0) |
			((cCounter << TSD_PLL_RECONFIG_C_COUNTER_SHFT) & TSD_PLL_RECONFIG_C_COUNTER_MASK));

	TSD_PLL_RECONFIG_IOWR_C_COUNTER(base, reg);

	return ((lowCnt >= 256) || (highCnt >= 256));
}

/*
 * Calculates the C counter for better resolution based on the bitrate.
 */
static inline
int calculate_c_counter(int referenceFreqHz, int newBitrateHz){
	int c;
	if(newBitrateHz < 5000000){
		c = (40*referenceFreqHz)/100000000;
	}else if(newBitrateHz < 10000000) {
		c = (20*referenceFreqHz)/100000000;
	}else if(newBitrateHz < 20000000) {
		c = (10*referenceFreqHz)/100000000;
	} else {
		c = (5*referenceFreqHz)/100000000;
	}
	if(c ==0) c = 1;
	return c;
}

/*
 * Calculates the m counter using the c counter, pll reference freq and the desired bitrate.
 */
static inline
int calculate_m_counter(int referenceFreqHz, int newBitrateHz, int cDivCounter){
    double d_pll_fin_hz = referenceFreqHz;
    double d_desired_hz = newBitrateHz;
    double mCounter = (100.0*cDivCounter*d_desired_hz)/d_pll_fin_hz;
    if(mCounter <= 0 ) {
        return 0;
    } else {
        return (int)((mCounter >= (floor(mCounter)+0.5f)) ? ceil(mCounter) : floor(mCounter));
    }
}

int reconfigure_pll_bitrate(uint32_t base, int referenceFreqHz, int newBitrateHz, ReconfigPLLInfo *pllInfo)
{
	int retval = TSD_PLL_RECONFIG_SUCCESS;
	int timeout = 1000;
	int mDivCounter;
	int cDivCounter = calculate_c_counter(referenceFreqHz, newBitrateHz);

	if((cDivCounter <= 0) || (cDivCounter > 255)) {
		return TSD_PLL_RECONFIG_CALC_FAIL;
	}
	mDivCounter = calculate_m_counter(referenceFreqHz, newBitrateHz, cDivCounter);

	if((mDivCounter <= 0) || (mDivCounter > 255)) {
		return TSD_PLL_RECONFIG_CALC_FAIL;
	}

	if(pll_reconfiguration_block_busy(base)){
		return TSD_PLL_RECONFIG_BUSY_FAIL;
	}

	// set reconfig to polling mode to determine when done.
	TSD_PLL_RECONFIG_IOWR_MODE(base, TSD_PLL_RECONFIG_MODE_POLLING);

	if(set_pll_n_counter(base, 50, 50, 0, 0)){
		retval = TSD_PLL_RECONFIG_OTHER_FAIL;
	}
	if(set_pll_m_counter(base, mDivCounter, mDivCounter, 0, 0)){
		retval = TSD_PLL_RECONFIG_OTHER_FAIL;
	}
	if(set_pll_c_counter(base, cDivCounter, cDivCounter, 0, 0, 0)){
		retval = TSD_PLL_RECONFIG_OTHER_FAIL;
	}

	TSD_PLL_RECONFIG_IOWR_BANDWIDTH(base, pll_get_bandwidth(mDivCounter));
	TSD_PLL_RECONFIG_IOWR_CHARGE_PUMP(base, pll_get_charge_pump(mDivCounter));

	if(pllInfo){
		pllInfo->cCounter = cDivCounter;
		pllInfo->mCounter = mDivCounter;
		pllInfo->nCounter = 50;
		pllInfo->kCounter = 0;
		pllInfo->bandwithVal = pll_get_bandwidth(mDivCounter);
		pllInfo->chargePumpVal = pll_get_charge_pump(mDivCounter);
		pllInfo->referencePllFreq = referenceFreqHz;
	}
	// start the reconfig
	TSD_PLL_RECONFIG_IOWR_START(base);

	// USER SHOULD WAIT FOR BUSY TO BE FALSE AT THIS POINT.
	return retval;
}

/*
 * returns a value indicating if the reconfiguration block located at
 * base address 'base' is currently busy.
 * return values:
 * false: the reconfiguration block is not busy.
 * true: the reconfiguration block is busy and cannot accept any commands.
 */
bool pll_reconfiguration_block_busy(uint32_t base)
{ /*

	 * For now, I am implementing the opposite. (1 = busy)
	 * This makes sense from the testing I have done.
	 */
	uint32_t stat = TSD_PLL_RECONFIG_IORD_STATUS(base);
	return stat ? true : false;
}

/*
 * Calculates that bitrate that the PLL should be producing given the input pllInfo.
 * returns the calculated bitrate.
 */
int pll_reconfiguration_get_frequency(ReconfigPLLInfo pllInfo)
{
	if((0 == pllInfo.cCounter) || (0 == pllInfo.referencePllFreq)) return 0;
	return pllInfo.mCounter * (pllInfo.referencePllFreq / (100 * pllInfo.cCounter));
}
