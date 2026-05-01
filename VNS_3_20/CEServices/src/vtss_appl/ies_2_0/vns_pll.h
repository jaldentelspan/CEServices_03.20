// ***************************************************************************
// vns_pll.h
// Copyright (c) [2006] - [2018]
//                                 Telspan Data, LLC. All rights reserved.
// Authored by: Jason Alden
// Description : To configure FPGA EPE PLL module
// Revision History
// As Documented in SVN Commit Logs
// ***************************************************************************
#ifndef VNS_PLL_HDLC_H
#define VNS_PLL_HDLC_H

/* PLL reconfig defines */
// #define PLL_BASE PLL_RECONFIG_BASE
// #define write_pll(REG, DATA) IOWR(PLL_BASE, REG, DATA)
// #define read_pll(REG) IORD(PLL_BASE, REG)
// #define PLL_BASE PLL_RECONFIG_BASE

/* Register addresses */
// #define PLL_MODE            (0x00)
// #define PLL_STATUS          (0x01)
// #define PLL_START           (0x02)
// #define PLL_N               (0x03)
// #define PLL_M               (0x04)
// #define PLL_C               (0x05)
// #define PLL_PHASE_SHIFT     (0x06)
// #define PLL_FRACT_M         (0x07)
// #define PLL_BANDWITH        (0x08)
// #define PLL_CHG_PUMP        (0x09)
// #define PLL_VCO_DIV         (0x1C)
// #define PLL_MIF_ADDR        (0x1F)
typedef enum {
    PLL_REG_MODE            = 0x00,
    PLL_REG_STATUS          = 0x01,
    PLL_REG_START           = 0x02,
    PLL_REG_N               = 0x03,
    PLL_REG_M               = 0x04,
    PLL_REG_C               = 0x05,
    PLL_REG_PHASE_SHIFT     = 0x06,
    PLL_REG_FRACT_M         = 0x07,
    PLL_REG_BANDWITH        = 0x08,
    PLL_REG_CHG_PUMP        = 0x09,
    PLL_REG_VCO_DIV         = 0x1C,
    PLL_REG_MIF_ADDR        = 0x1F,
    PLL_REG_END
} pll_registers_t;
/* PLL Masks */
#define PLL_MODE_MSK        ( 0x01 << 0 )
#define PLL_STAT_MSK        ( 0x01 << 0 )
#define PLL_START_MSK       ( 0x01 << 0 )
#define PLL_BANDWIDTH_MSK   ( 0x0F << 0 )
#define PLL_CHG_PUMP_MSK    ( 0x03 << 0 )
#define PLL_VCO_DIV_MSK     ( 0x01 << 0 )

#define PLL_N_LO_MSK        ( 0xFF << 0  )
#define PLL_N_HI_MSK        ( 0xFF << 8  )
#define PLL_N_BYPASS        ( 0x01 << 16 )
#define PLL_N_ODDDIV        ( 0x01 << 17 )

#define PLL_M_LO_MSK        ( 0xFF << 0  )
#define PLL_M_HI_MSK        ( 0xFF << 8  )
#define PLL_M_BYPASS        ( 0x01 << 16 )
#define PLL_M_ODDDIV        ( 0x01 << 17 )

#define PLL_C_LO_MSK        ( 0xFF << 0  )
#define PLL_C_HI_MSK        ( 0xFF << 8  )
#define PLL_C_BYPASS        ( 0x01 << 16 )
#define PLL_C_ODDDIV        ( 0x01 << 17 )
#define PLL_C_SEL           ( 0x1F << 18 )

#define PLL_N_LO_DEFAULT 10
#define PLL_N_HI_DEFAULT 10
#define PLL_C_LO_DEFAULT 10
#define PLL_C_HI_DEFAULT 10
#define PLL_M_LO_DEFAULT 10
#define PLL_M_HI_DEFAULT 10

#define PLL_POLLING_MODE 1
/* PLL related functions */
/* get value for bandwidth register based on M counter val */
static inline
int pll_get_bandwidth(int M) {
    /* derived from plugging in numbers into Altera's PLL reconf spreadsheet */
    if(M >= 100) {
        return 2;
    } else if(M >= 70) {
        return 3;
    } else if(M >= 40) {
        return 4;
    } else if(M >= 20) {
        return 6;
    } else if(M >= 10) {
        return 7;
    } else {
        return 8;
    }
}

/* get value for charge pump register based on M counter val */
static inline
int pll_get_charge_pump(int M) {
    /* derived from plugging in numbers into Altera's PLL reconf spreadsheet */
    if(M >= 10) {
        return 1;
    } else {
        return 0;
    }
}

/* percent = (100*fout)/fin */
static inline
int pll_khz_to_percent(int pll_fin_khz, int desired_khz) {
    double d_pll_fin_khz = pll_fin_khz;
    double d_desired_khz = desired_khz;
    double percent = (100.0*d_desired_khz)/d_pll_fin_khz;
    if(percent <= 0 ) {
        return 0;
    } else if ( percent > 250 ) {
        return 250;
    } else {
        return (int)percent;
    }
}

/* percent = (100*fout)/fin */
static inline
int pll_hz_to_percent(int pll_fin_hz, int desired_hz) {
    double d_pll_fin_hz = pll_fin_hz;
    double d_desired_hz = desired_hz;
    double percent = (100.0*d_desired_hz)/d_pll_fin_hz;
    if(percent <= 0 ) {
        return 0;
    } else if ( percent > 250 ) {
        return 250;
    } else {
        return (int)percent;
    }
}

/* fout = fin * percent * 100 */
static inline
int pll_percent_to_khz(int pll_fin_khz, int percent) {
    double d_pll_fin_khz = pll_fin_khz;
    double d_percent = percent;
    double tmp = (d_pll_fin_khz*d_percent)/100.0;
    return (int)tmp;
}

/* fout = fin * percent * 100 */
static inline
int pll_percent_to_hz(int pll_fin_hz, int percent) {
    double d_pll_fin_hz = pll_fin_hz;
    double d_percent = percent;
    double tmp = (d_pll_fin_hz*d_percent)/100.0;
    return (int)tmp;
}

int __ffs(register uint32_t input, register uint32_t half) {
    register uint32_t top = (input >> half);
    register uint32_t bot = (input & (-1UL >> (32 - half)));
    register uint32_t answer;

    if (!input) {
        return 0;
    }

    if (half == 1) {
        return (bot ? bot : top + half);
    }

    if ((answer = __ffs(bot, half >> 1))) {
        return answer;
    } else {
        return (__ffs(top, half >> 1) + half);
    }
}

int _ffs(register uint32_t input) {
    return __ffs(input, 16);
}
uint32_t SETMSK32(register uint32_t reg, register uint32_t mask, register int32_t value) {
    reg &= ~mask;  // Clear previous mask bits
    reg |= mask & (value << (_ffs(mask) - 1));
    return reg;
}

#endif /* VNS_PLL_HDLC_H */
