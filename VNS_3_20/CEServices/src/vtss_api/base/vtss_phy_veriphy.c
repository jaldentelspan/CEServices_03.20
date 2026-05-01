/*

 Vitesse PHY API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
 Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted. Permission to
 integrate into other products, disclose, transmit and distribute the software
 in an absolute machine readable format (e.g. HEX file) is also granted.  The
 source code of the software may not be disclosed, transmitted or distributed
 without the written permission of Vitesse. The software and its source code
 may only be used in products utilizing the Vitesse switch products.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software. Vitesse retains all ownership,
 copyright, trade secret and proprietary rights in the software.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
 INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR USE AND NON-INFRINGEMENT.

 $Id$
 $Revision$

*/




#include "vtss_options.h"

#ifdef VTSS_CHIP_CU_PHY

#define VTSS_TRACE_GROUP VTSS_TRACE_GROUP_PHY

#include "vtss_api.h"
#include "vtss_phy.h"
#include "vtss_state.h"

#define SmiWrite(phy, reg, val) vtss_phy_wr(phy, reg, val)


#define ExtMiiWrite(phy, reg, val) SmiWrite(phy, 31, VTSS_PHY_PAGE_EXTENDED); SmiWrite(phy, reg, val)

#define VTSS_PHY_1_GEN_DSP(port_no) ((port_family(port_no) == VTSS_PHY_FAMILY_MUSTANG) || \
                                    (port_family(port_no) == VTSS_PHY_FAMILY_COBRA))


static int SmiRead(int port_no, int reg)
{
    u16 rv;
    vtss_phy_rd(port_no, reg, &rv);
    return rv;
}


static int ExtMiiReadBits(const vtss_port_no_t port, char reg, char msb, char lsb)
{
    int x;

    vtss_phy_page_ext(port);
    x = SmiRead(port, reg);
    if (msb < 15)
        x &= (1 << (msb + 1)) - 1;
    x = (int)((unsigned int) x >> lsb);

    return x;
}

static int MiiReadBits(const vtss_port_no_t port, char reg, char msb, char lsb)
{
    int x;

    vtss_phy_page_std(port);
    x = SmiRead(port, reg);
    if (msb < 15)
        x &= (1 << (msb + 1)) - 1;
    x = (int)((unsigned int) x >> lsb);

    return x;
}

static void MiiWriteBits(const vtss_port_no_t port, char reg, char msb, char lsb, i8 value)
{
    int x;
    unsigned int t = 1, mask;

    vtss_phy_page_std(port);
    x = SmiRead (port, reg);
    mask = (((t << (msb-lsb+1))-t)<<lsb);
    x = ((value<<lsb) & mask) | (x & (~mask));
    SmiWrite (port, reg, x);
}

static int TpReadBits(const vtss_port_no_t port, char reg, char msb, char lsb)
{
    int x;

    vtss_phy_page_test(port);
    x = SmiRead(port, reg);
    if (msb < 15)
        x &= (1 << (msb + 1)) - 1;
    x = (int)((unsigned int) x >> lsb);

    return x;
}

static void TpWriteBit(const vtss_port_no_t port, i8 TpReg, i8 bitNum, i8 value)
{
    short val;

    vtss_phy_page_test(port);
    val = SmiRead(port, TpReg);
    if (value)
        val = val | (1 << bitNum);
    else
        val = val & ~(1 << bitNum);
    vtss_phy_wr(port, TpReg, val);
}


static vtss_phy_family_t port_family(vtss_port_no_t port_no)
{
    return vtss_state->phy_state[port_no].family;
}

static vtss_rc tr_raw_read(const vtss_port_no_t port, const u16 TrSubchanNodeAddr, u32 *tr_raw_data)
{
    u16 x;
    vtss_mtimer_t timer;
    u32 base;

    
    
    base = VTSS_PHY_1_GEN_DSP(port) ? 0 : 16;
    VTSS_RC(vtss_phy_wr(port, base, (5 << 13) | TrSubchanNodeAddr));
    VTSS_MTIMER_START(&timer,500);

    while (1) {
        VTSS_RC(vtss_phy_rd(port, base, &x));
        if (x & 0x8000)
            break;
        if (VTSS_MTIMER_TIMEOUT(&timer)) {
            VTSS_MTIMER_CANCEL(&timer);
            return VTSS_RC_ERROR; 
        }
    }
    VTSS_MTIMER_CANCEL(&timer);

    vtss_phy_rd(port, base + 2, &x); 
    *tr_raw_data = (u32) x << 16;
    vtss_phy_rd(port, base + 1, &x); 
    *tr_raw_data |= x;

    return VTSS_RC_OK;
}

static vtss_rc tr_raw_write(u8 port_no, u16 ctrl_word, u32 val)
{
    u32 base;

    
    
    base = VTSS_PHY_1_GEN_DSP(port_no) ? 0 : 16;

    VTSS_RC(vtss_phy_wr(port_no, base + 2, val >> 16));   
    VTSS_RC(vtss_phy_wr(port_no, base + 1, (u16) val));  
    VTSS_RC(vtss_phy_wr(port_no, base, (4 << 13) | ctrl_word));

    return VTSS_RC_OK;
}



static vtss_rc tr_raw_long_read(u8 subchan_phy, const u16 TrSubchanNode)
{
    u16 x, phy_num;
    vtss_mtimer_t timer;

    phy_num = subchan_phy & 0x3f;
    vtss_phy_page_tr(phy_num);
    VTSS_RC(vtss_phy_wr(phy_num, 0, TrSubchanNode));

    VTSS_MTIMER_START(&timer,500);
    while (1) {
        VTSS_RC(vtss_phy_rd(phy_num, 0, &x));
        if (x & 0x8000)
           break;
        if (VTSS_MTIMER_TIMEOUT(&timer)) {
            VTSS_MTIMER_CANCEL(&timer);
            return VTSS_RC_ERROR; 
        }
    }
    VTSS_MTIMER_CANCEL(&timer);
    return VTSS_RC_OK;
}


static vtss_rc process_ec_result (u8 subchan_phy, u16 tap, u32 *tr_raw_data)
{
    u16 reg_beg, reg_end, reg_beg_msb, reg_end_lsb;
    u16 x, y, phy_num;
    u32 ret;
    u16 firstbit, bitwidth;
    phy_num = subchan_phy & 0x3f;

    firstbit = (subchan_phy >> 6) * 68;
    bitwidth = 15;

    if (tap < 16) {
        bitwidth = 20;
    } else {
        firstbit += 20;
        if (tap < 32) {
            bitwidth = 18;
        } else {
            firstbit += 18;
            if (tap >= 48) {
                firstbit += 15;
            }
        }
    }
    reg_beg = 20 - (firstbit >> 4);
    reg_beg_msb = 15 - (firstbit & 0xf);
    reg_end = 20 - ((firstbit + bitwidth -1)>>4);
    reg_end_lsb = 15 - ((firstbit + bitwidth -1) & 0xf);

    if (reg_beg == reg_end) {
        vtss_phy_rd (phy_num, reg_beg, &x);
        x &= (1 << (reg_beg_msb +1)) - 1;
        ret = x >> reg_end_lsb;
    } else if (reg_beg == (reg_end + 1)) {
        vtss_phy_rd (phy_num, reg_beg, &x);
        x &= (1 << (reg_beg_msb + 1)) - 1;
        ret = (u32)x << (16 - reg_end_lsb);
        vtss_phy_rd (phy_num, reg_end, &y);
        y >>= reg_end_lsb;
        ret += y;
    } else {
        
        return -1;
    }
    if (ret >= (1L << (bitwidth-1))) {
        ret -= (1L << bitwidth);
    }

    *tr_raw_data = ret;

    return VTSS_RC_OK;
}


static vtss_rc process_var_result(u8 subchan_phy, u16 flt, u32 *tr_raw_data)
{
    u16 reg_beg, reg_end, reg_beg_msb, reg_end_lsb;
    u16 x, y, phy_num, sub_chan, scl=0;
    u16 firstbit, bitwidth;

    reg_beg=reg_end=reg_beg_msb=reg_end_lsb=x=0;
    phy_num = subchan_phy & 0x3f;
    sub_chan =     subchan_phy >> 6;
    bitwidth = 15;

    switch (flt) {
        case 0:        
        case 1:        
        case 2:        
            scl = (sub_chan<<2)-sub_chan+flt;
            break;
        case 3:        
            scl = sub_chan+(((flt+1)<<2)-(flt+1));
            break;
        default:    
            break;
    }
    firstbit = (scl<<4)-scl;

    reg_beg = 20 - (firstbit >> 4);
    reg_beg_msb = 15 - (firstbit & 0xf);
    reg_end = 20 - ((firstbit + bitwidth -1)>>4);
    reg_end_lsb = 15 - ((firstbit + bitwidth -1) & 0xf);

    if (reg_beg == reg_end) {
        vtss_phy_rd (phy_num, reg_beg, &x);
        x &= (u16)((1L << (reg_beg_msb +1)) - 1);
        x >>= reg_end_lsb;
    } else if (reg_beg == (reg_end + 1)) {
        vtss_phy_rd (phy_num, reg_beg, &x);
        x &= (u16)((1L << (reg_beg_msb + 1)) - 1);
        x <<= 16 - reg_end_lsb;
        vtss_phy_rd (phy_num, reg_end, &y);
        y >>= reg_end_lsb;
        x += y;
    } else {
        
        return -1;
    }
    if (x >= (1 << (bitwidth-1))) {
        x -= (1 << bitwidth);
    }

    *tr_raw_data = (u32)((long)((short)x));

    return VTSS_RC_OK;
}
static vtss_rc vtss_phy_1_gen_pre_veriphy (vtss_phy_family_t family, vtss_veriphy_task_t c51_idata *tsk)
{
    u32 x;
    BOOL conf_none;

    tsk->saveReg = 0;
    tsk->tokenReg = 0;

    VTSS_D("family = %d\n", family);
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        
        
        tsk->saveReg |= SmiRead (tsk->port, 0);
        tsk->saveReg |= ((MiiReadBits (tsk->port, 9, 12, 11))<< 16);
        tsk->saveReg |= ((MiiReadBits (tsk->port, 28, 2, 2)) << 18);
        MiiWriteBits (tsk->port, 28, 2, 2, 1);    
        tsk->saveReg |= ((TpReadBits (tsk->port, 5, 4, 3))<< 19);
    } else  {
        
        
        tsk->saveReg |= (MiiReadBits (tsk->port, 0, 14, 6));
        tsk->saveReg |= ((MiiReadBits (tsk->port, 4, 11, 5)) << 9);
        tsk->saveReg |= ((MiiReadBits (tsk->port, 9, 12, 8)) << 16);
        tsk->saveReg |= ((MiiReadBits (tsk->port, 23, 2, 1)) << 21);
        tsk->saveReg |= ((MiiReadBits (tsk->port, 23, 15, 12)) << 23);

        VTSS_RC(vtss_phy_page_std (tsk->port));
        





        VTSS_RC(vtss_phy_wr_masked(tsk->port, 23, 0x1004, 0xf006));
        conf_none = vtss_state->phy_state[tsk->port].conf_none;
        vtss_state->phy_state[tsk->port].conf_none = 0;
        VTSS_RC(vtss_phy_reset_private(tsk->port));
        vtss_state->phy_state[tsk->port].conf_none = conf_none;
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 28, 0x0000, 0x0040));
    }
    MiiWriteBits (tsk->port, 9, 12, 11, 3);    
    SmiWrite (tsk->port, 0, 0x0140);           

    
    vtss_phy_page_test(tsk->port);
    TpWriteBit (tsk->port, 5, 4, 1);     
    TpWriteBit (tsk->port, 5, 3, 1);

    
    
    
    
    
    
    
    
    vtss_phy_page_tr(tsk->port);

    
    tr_raw_read (tsk->port, 0xf8a , &x);
    x = (x & ~0x1fc)|0x108;
    tr_raw_write (tsk->port, 0xf8a, x);

    
    tr_raw_read (tsk->port, 0x1680 , &x);
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tsk->tokenReg = ((x>>11) & 0xfff);
    }
    x = (x & ~0x3ffc00L) | 0x3ffc00L;
    tr_raw_write (tsk->port, 0x1680, x);

    
    
    tr_raw_read (tsk->port, 0xf86 , &x);
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tsk->tokenReg |= ((x >> 11) & 0x7ff) << 12;
    }
    x = (x & ~0x3ff800L) | 0x3a1000L;
    tr_raw_write (tsk->port, 0xf86, x);

    
    tr_raw_read (tsk->port, 0xf88 , &x);
    x = (x & ~0x1f00) | 0x1200;
    tr_raw_write (tsk->port, 0xf88, x);

    
    tr_raw_read (tsk->port, 0xf82 , &x);
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tsk->tokenReg |= ((x >> 3) & 3) << 23;
    }
    x = (x & ~0x180) | 0x100;
    tr_raw_write (tsk->port, 0xf82, x);

    
    tr_raw_read (tsk->port, 0xfa0 , &x);
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tsk->tokenReg |= ((x >> 8) & 0x1f) << 25;
    }
    x = (x & ~0x7f00)|0x6800;
    tr_raw_write (tsk->port, 0xfa0, x);

    
    tr_raw_read (tsk->port, 0xfa2 , &x);
    x = (x & ~0x30303) | 0x30303;
    tr_raw_write (tsk->port, 0xfa2, x);

    
    tr_raw_read (tsk->port, 0x240 , &x);
    x = (x & ~0xff) | 0xff;
    tr_raw_write (tsk->port, 0x240, x);

    
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tr_raw_read (tsk->port, 0x770 , &x);
        x = (x & ~0x0c) | 8;
        tr_raw_write (tsk->port, 0x770, x);
    } else  {
        tr_raw_read (tsk->port, 0x708, &x);
        x = (x & ~0xff0) | 0xaa0;
        tr_raw_write (tsk->port, 0x708, x);
    }

    
    VTSS_RC(vtss_phy_page_std(tsk->port));
    MiiWriteBits (tsk->port, 0, 11, 11, 1);        
    MiiWriteBits (tsk->port, 0, 11, 11, 0);        

    vtss_phy_page_tr (tsk->port);
    
    tr_raw_read (tsk->port, 0x1684, &x);
    x = (x & ~0x3fc000L) | 0x3c0000L;
    tr_raw_write (tsk->port, 0x1684, x);

    
    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        tr_raw_read (tsk->port, 0x0770 , &x);
        x = (x & ~3) | 3;
        tr_raw_write (tsk->port, 0x0770, x);
    } else  {
        tr_raw_read (tsk->port, 0x708, &x);
        x = (x & ~0x0c) | 0x0c;
        tr_raw_write (tsk->port, 0x708, x);
    }
    return vtss_phy_page_std(tsk->port);
}

static vtss_rc vtss_phy_1_gen_post_veriphy (vtss_phy_family_t family, vtss_veriphy_task_t c51_idata *tsk)
{
    u32 x;
    BOOL conf_none;

    if (family == VTSS_PHY_FAMILY_MUSTANG) {
        
        
        
        
        
        
        

        vtss_phy_page_tr(tsk->port);
        \
        tr_raw_read (tsk->port, 0x770 , &x);
        x = (x & ~0xf);
        tr_raw_write (tsk->port, 0x770, x);

        
        tr_raw_read (tsk->port, 0xf8a, &x);
        x = (x & ~0x1fc);
        tr_raw_write (tsk->port, 0xf8a, x);

        
        tr_raw_read (tsk->port, 0x1680, &x);
        x = (x & ~0x3ffc00) | ((tsk->tokenReg  & 0xfff) << 10);
        tr_raw_write (tsk->port, 0x1680, x);

        
        
        tr_raw_read (tsk->port, 0xf86, &x);
        x = (x & ~0x3ff800) | (((tsk->tokenReg >> 12) & 0x7ff) << 11);
        tr_raw_write (tsk->port, 0xf86, x);

        
        tr_raw_read (tsk->port, 0xf88, &x);
        x = (x & ~0x1f00);
        tr_raw_write (tsk->port, 0xf88, x);

        
        tr_raw_read (tsk->port, 0xf82, &x);
        x = (x & ~0x18) | (((tsk->tokenReg >> 23) & 0x3) << 3);
        tr_raw_write (tsk->port, 0xf82, x);
        
        tr_raw_read (tsk->port, 0xfa0, &x);
        x = (x & ~0x7f00) | (((tsk->tokenReg >> 25) & 0x1f) << 8);
        tr_raw_write (tsk->port, 0xfa0, x);

        
        tr_raw_read (tsk->port, 0xfa2, &x);
        x = (x & ~0x30303);
        tr_raw_write (tsk->port, 0xfa2, x);

        
        tr_raw_read (tsk->port, 0x240, &x);
        x = (x & ~0xff);
        tr_raw_write (tsk->port, 0x240, x);

        
        tr_raw_read (tsk->port, 0x1684, &x);
        x = (x & ~0x3fc000L);
        tr_raw_write (tsk->port, 0x1684, x);

        
        TpWriteBit (tsk->port, 5, 4, ((tsk->saveReg>>20)&1));
        TpWriteBit (tsk->port, 5, 3, ((tsk->saveReg>>19)&1));
        MiiWriteBits (tsk->port, 9, 12, 11, ((tsk->saveReg>>16)&3));
        SmiWrite (tsk->port, 0, (tsk->saveReg & 0xffff));
        MiiWriteBits (tsk->port, 28, 2, 2, ((tsk->saveReg>>18)&1));
        MiiWriteBits (tsk->port, 0, 11, 11, 1);        
        MiiWriteBits (tsk->port, 0, 11, 11, 0);        
    } else  {
        u16 reg23 = 0;

        VTSS_RC(vtss_phy_page_test(tsk->port));
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 8, 0, 0x0002));
        VTSS_RC(vtss_phy_page_std(tsk->port));
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 22, 0, 0x0200));
        

        
        reg23 = ((tsk->saveReg & 0x7800000L) >> 11) | ((tsk->saveReg & 0x600000L) >> 20);
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 23, reg23, 0xf006));
        
        conf_none = vtss_state->phy_state[tsk->port].conf_none;
        vtss_state->phy_state[tsk->port].conf_none = 0;
        VTSS_RC(vtss_phy_reset_private(tsk->port));
        vtss_state->phy_state[tsk->port].conf_none = conf_none;
        
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 9, ((tsk->saveReg & 0x1f0000L) >> 8), 0x1f00));
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 4, (((tsk->saveReg & 0xfe00L) >> 9) << 5), 0xfe0));
        VTSS_RC(vtss_phy_wr_masked(tsk->port, 0, ((tsk->saveReg & 0x1ffL) << 6), 0x7fc0));
    }
    return vtss_phy_page_std(tsk->port);
}


static void get_anom_thresh(vtss_veriphy_task_t c51_idata *tsk, u8 tap)
{
    short log2ThreshX256;
    register i8 sh;

    log2ThreshX256 = 542 - (8 * (short)tap) + tsk->log2VGAx256;
    if (tsk->nc != 0)
        log2ThreshX256 -= 500;
    tsk->thresh[0] = 256 + (log2ThreshX256 & 255);
    sh = (i8)(log2ThreshX256 >> 8);
    if (sh >= 0) {
        tsk->thresh[0] <<= sh;
    }
    else {
        tsk->thresh[0] >>= -sh;
        if (tsk->thresh[0] < 23)
            tsk->thresh[0] = 23;
    }

    if (tsk->flags & 1) {
        tsk->thresh[1]  = tsk->thresh[0] >> 1;    
    }
    else {
        u8 idx;

        tsk->thresh[1] = 0;
        for (idx = 0; idx < 5; ++idx) { 
            tsk->thresh[1] = (tsk->thresh[1] + tsk->thresh[0] + 2) >> 2;
        }
    }
    if (tsk->thresh[1] < 15)
        tsk->thresh[1] = 15;

    
    if ((tsk->thresh[1] > 1012) && ((tsk->nc > 0) || (tap >= 32))) {
        tsk->thresh[1] = 1012;
    }
    else if ((tsk->thresh[1] > 4084) && (tap >= 16)) {
        tsk->thresh[1] = 4084;
    }
    else if (tsk->thresh[1] > 8180) {
        tsk->thresh[1] = 8180;
    }
    VTSS_D("get_anom_thresh, thresh[0]=%d, thresh[1]=%d\n", tsk->thresh[0], tsk->thresh[1]);
}

static short maxAbsCoeff;
static short c51_idata coeff[12];                 




static short c51_idata *readAvgECNCECVar(u8 subchan_phy, u8 first, u8 rpt_numCoeffs)
{
    VTSS_D("readAvgECNCECVar(%d%c, %d #%d, rpt %d, discard=%d)\n", subchan_phy & 0x3f, ('A'+(subchan_phy >> 6)), first, \
                                    rpt_numCoeffs & 15, ((rpt_numCoeffs >> 4) & 7) + 1, (rpt_numCoeffs & DISCARD) ? 1 : 0);

    
    if (VTSS_PHY_1_GEN_DSP(subchan_phy&0x3f)) {
        long c;
        u8 i, j, discrd_flg=0, flt, numRpt=0;
        i8 preScale;
        u16 TrSubchanNode=0, SubchanNode=0;
        u32 tr_raw_data = 0;

        VTSS_N("readAvgECNCECVar(%d%c, %d #%d, %d) returns ", subchan_phy & 0x3f, ('A'+(subchan_phy >> 6)), first, rpt_numCoeffs & 7, rpt_numCoeffs >> 3);
        maxAbsCoeff = c = 0;
        numRpt = ((rpt_numCoeffs >> 4) & 7) + 1;

        if (rpt_numCoeffs & DISCARD) {
            discrd_flg = 1;
            rpt_numCoeffs &= ~DISCARD;
        }

        if (!discrd_flg) {
            
            for (j = 0; j < 12; ++j)
                coeff[j] = 0;
        }

        
        if (first & 0x80) {
            flt = first >> 6;
            if (flt == 2) {    
                SubchanNode = 7<<13;
                VTSS_D("ECHO CANCELER, Sub-channel=%d, first tap=%d\n", (subchan_phy>>6), (first & 0x3f));
            } else if (flt == 3) { 
                SubchanNode = 0x1c1<<7;
                VTSS_D("ECVar, Sub-channel=%d, first tap=%d\n", (subchan_phy>>6), (first & 0x3f));
            }
        }
        else { 
            flt = first >> 5;
            SubchanNode = 0x1c1 << 7;
            VTSS_D("NC%d, Sub-channel=%d, first tap=%d\n", (flt+1), (subchan_phy>>6), (first & 0x1f));
        }

        
        preScale = 0;
        if ((first & 0x80) && (flt == 2)) {        
            if ((first & 0x3f) < 16) {
                if (numRpt > 4)
                    preScale = 3;
                else if (numRpt > 2)
                    preScale = 2;
                else if (numRpt > 1)
                    preScale = 1;
            }
            else if ((first & 0x3f) < 32) {
                if (numRpt > 4 )
                    preScale = 1;
            }
        }

        
        for (i = 0; i < numRpt; ++i) {
            for (j = 0; j < (rpt_numCoeffs & 0x0f); ++j) {
                if ((first & 0x80) && (flt == 2)) {    
                    TrSubchanNode = SubchanNode|((32+((first+j)&0xf))<<1);
                    tr_raw_long_read (subchan_phy, TrSubchanNode);
                    process_ec_result(subchan_phy, (first & 0x3f)+j, &tr_raw_data);
                } else {    
                    TrSubchanNode = SubchanNode|((32+((first & 0x1f)+j))<< 1);
                    tr_raw_long_read (subchan_phy, TrSubchanNode);
                    process_var_result(subchan_phy, flt, &tr_raw_data);
                }

                
                c = (long)tr_raw_data;

                if (c > 0) {
                    if ((c >> 4) > maxAbsCoeff)
                        maxAbsCoeff = c >> 4;
                }
                else if (-(c >> 4) > maxAbsCoeff)
                    maxAbsCoeff = -(c >> 4);

                if (!discrd_flg) {
                    
                    
                    coeff[j] += (c + (1 << (3 + preScale))) >> (4 + preScale);
                }
            }
        }

        
        if (!discrd_flg) {
            if (numRpt == 3) {
                for (j = 0; j <= (rpt_numCoeffs & 0xf); ++j) {
                    c = ((long)coeff[j]) << preScale;
                    for (i = 0; i < 4; ++i)
                        c = (c >> 2) + ((long)coeff[j] << preScale);
                    coeff[j] = (c + 2) >> 2;
                }
            }
            else {        
                for (i = -preScale, j = numRpt; j > 1; ++i, j = j >> 1)
                    ;

                if (i > 0) {
                    for (j = 0; j <= (rpt_numCoeffs & 0xf ); ++j) {
                        coeff[j] = (coeff[j] + (1 << (i-1))) >> i;
                    }
                }
            }
        }

        VTSS_D("{");
        for (j = 0; j < (rpt_numCoeffs & 0xf); ++j) {
            VTSS_D(" %d", coeff[j]);
        }
        VTSS_D(" }, ");
        VTSS_D("maxAbsCoeff = %d\n", maxAbsCoeff);
    } else {
        
        i8 i, preScale=0;
        short c;
        u8 j, numRpt;
        u32 tr_raw_data;
        BOOL discrd_flg=0;

        VTSS_N("readAvgECNCECVar(%d%c, %d #%d, rpt %d, discard=%d)\n", subchan_phy & 0x3f, ('A'+(subchan_phy >> 6)), first, rpt_numCoeffs & 15, ((rpt_numCoeffs >> 4) & 7) + 1, (rpt_numCoeffs & DISCARD) ? 1 : 0);

        maxAbsCoeff = 0;
        numRpt = ((rpt_numCoeffs >> 4) & 7) + 1;

        if (rpt_numCoeffs & DISCARD) {
            preScale = -1;
            discrd_flg = 1;
            rpt_numCoeffs &= ~DISCARD;
        }

        if (!discrd_flg) {
            
            preScale = (first < 16 && numRpt >= 8) ? 1 : 0;

            
            for (j = 0; j < 8; ++j)
                coeff[j] = 0;
        }

        
        for (i = 0; i < numRpt; ++i) {
            for (j = 0; j < (rpt_numCoeffs & 15); ++j) {
                tr_raw_read(subchan_phy & 0x3f, ((u16)(subchan_phy & 0xc0) << 5)
                            | ((u16)(j + first) << 1), &tr_raw_data);
                if (tr_raw_data & 0x2000 )
                    c = 0xc000 | (short)tr_raw_data;
                else
                    c = (short)tr_raw_data;

                if (c > 0) {
                    if (c > maxAbsCoeff)
                        maxAbsCoeff = c;
                }
                else if (-c > maxAbsCoeff)
                    maxAbsCoeff = -c;

                if (preScale >= 0)
                    coeff[j] += c >> preScale;
            }
        }

        if (preScale >= 0) {
            if (numRpt == 3) {
                
                for (j = 0; j < (rpt_numCoeffs & 15); ++j) {
                    c = coeff[j];
                    for (i = 0; i < 4; ++i)
                        c = (c >> 2) + coeff[j];
                    coeff[j] = (c + 2) >> 2;
                }
            }
            else {
                
                for (i = 0, j = numRpt >> preScale; j > 1; ++i, j = j >> 1)
                    ;

                
                for (j = 0; j < (rpt_numCoeffs & 15); ++j)
                    coeff[j] = coeff[j] >> i;
            }
            VTSS_D("{");
            for (j = 0; j < (rpt_numCoeffs & 15); ++j) {
                VTSS_D(" %d", coeff[j]);
            }
            VTSS_D(" }, ");
        }
        VTSS_D("maxAbsCoeff = %d\n", maxAbsCoeff);
    }
    
    return &coeff[0];
}


static BOOL checkValidity(vtss_veriphy_task_t c51_idata *tsk, short noiseLimit)
{
    vtss_mtimer_t timer;
    i8 timeout = 0;
    vtss_phy_family_t family;

    if (tsk->flags & 1)
        return 1;

    family = port_family(tsk->port);

    vtss_phy_page_tr(tsk->port);
    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
    
        
        tr_raw_write(tsk->port, 0x0b6, (((255-64) << 2) | 3));
        tr_raw_write(tsk->port, 0x0b6, (((255-64) << 2) | 2)); 
    }

    if (family == VTSS_PHY_FAMILY_QUATTRO || 
        family == VTSS_PHY_FAMILY_SPYDER ||  
        family == VTSS_PHY_FAMILY_ENZO) {
        
        tr_raw_write(tsk->port, 0x0188, (tsk->tr_raw0188 & 0xfffe00) | (160 << 1) | 1);
        tr_raw_write(tsk->port, 0x0184, 0x02 << tsk->subchan); 
        tr_raw_write(tsk->port, 0x0184, 0); 
    }

    VTSS_MSLEEP(1);
    
    if (!VTSS_PHY_1_GEN_DSP(tsk->port)) {
        tr_raw_write(tsk->port, 0x0188, tsk->tr_raw0188); 
    }
    VTSS_MTIMER_START(&timer, 200);
    while (1) {
        if ((readAvgECNCECVar(
                 (tsk->subchan << 6) | tsk->port,
                 
                 VTSS_PHY_1_GEN_DSP(tsk->port) ? 192 : 72,
                 0xb8), maxAbsCoeff) >= ((tsk->stat[(int)tsk->subchan] == 0) ? 4 : 1))
            break;
        if (VTSS_MTIMER_TIMEOUT(&timer)) {
            timeout = 1;
            break;
        }
    }
    VTSS_MTIMER_CANCEL(&timer);
    
    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
        tr_raw_write(tsk->port, 0x0b6, tsk->tr_raw0188 | 1); 
        tr_raw_write(tsk->port, 0x0b6, tsk->tr_raw0188 | 0); 
    } else {
        tr_raw_write(tsk->port, 0x0184, 0x02 << tsk->subchan); 
        tr_raw_write(tsk->port, 0x0184, 0); 
    }

    if (timeout) {
        if (tsk->stat[(int)tsk->subchan] != 0) {  
            tsk->stat[(int)tsk->subchan]      = 0;
            tsk->loc[(int)tsk->subchan]       = 0;
            tsk->strength[(int)tsk->subchan]  = 0;
            VTSS_D("Invalid Anomaly Found");
        }
        return 0;
    }
    else if (maxAbsCoeff > noiseLimit) {
        if (tsk->stat[(int)tsk->subchan] == 0) {  
            tsk->loc[(int)tsk->subchan] = 255;    
            tsk->strength[(int)tsk->subchan] = maxAbsCoeff;
            VTSS_D("Length Invalid??? Block off this channel contribution...");
        }
        else {                        
            tsk->stat[(int)tsk->subchan]      = 0;
            tsk->loc[(int)tsk->subchan]       = 0;
            tsk->strength[(int)tsk->subchan]  = 0;
            VTSS_D("Invalid Anomaly");
        }
        return 0;
    }
    else if (tsk->stat[(int)tsk->subchan] == 0 && (maxAbsCoeff + 2) >= ABS(tsk->strength[(int)tsk->subchan]))  {
        tsk->thresh[(int)tsk->subchan]   = (maxAbsCoeff + 2);
        tsk->loc[(int)tsk->subchan]      = 0;
        tsk->strength[(int)tsk->subchan] = 0;
        VTSS_D(" thresh[%d] = %d, loc[%d] =0", tsk->subchan, maxAbsCoeff+2, tsk->subchan);
        return 1;
    }
    else {
        return 10;
    }
    return 1;
}



static void xc_search(vtss_veriphy_task_t c51_idata *tsk, u8 locFirst, u8 prefix)
{
    u8 idx;
    short s;
    short c51_idata *c;

    VTSS_D("xc_search: locFirst = %d, prefix = %d", locFirst, prefix);
    vtss_phy_page_tr(tsk->port);
    c = readAvgECNCECVar((tsk->subchan << 6) | tsk->port, tsk->firstCoeff,
                         (3 << 4) | tsk->numCoeffs);

    for (idx = (tsk->numCoeffs - 1), c += (tsk->numCoeffs - 1); (i8)idx >= 0; --idx, --c) {

        s = (short)(32L*(long)*c/(long)tsk->thresh[1]);
        VTSS_D("xc_search: Strength = %d", s);

        if (ABS(*c) > tsk->thresh[1]) {
            if ((ABS(s) <= ABS(tsk->strength[(int)tsk->subchan]))) {
                if ((tsk->signFlip < 0) && (tsk->stat[(int)tsk->subchan] < 4)
                    && (tsk->loc[(int)tsk->subchan] <= (locFirst + idx + 3))) {
                    if ((*c > 0) && (tsk->strength[(int)tsk->subchan] < 0)) {
                        tsk->stat[(int)tsk->subchan] = 2;
                        tsk->signFlip = 2;
                        if ((locFirst + idx) < prefix) {
                            tsk->loc[(int)tsk->subchan] = locFirst + idx;
                        }
                        VTSS_D("\txc_search: Open-->short flip @ tap %d, strength = %d\n", locFirst + idx, s);
                    }
                    else if ((*c < 0) && (tsk->strength[(int)tsk->subchan] > 0)) {
                        tsk->stat[(int)tsk->subchan] = 1;
                        tsk->signFlip = 2;
                        if ((locFirst + idx) < prefix) {
                            tsk->loc[(int)tsk->subchan] = locFirst + idx;
                        }
                        VTSS_D("\txc_search: Short-->open flip @ tap %d, strength = %d\n", locFirst + idx, s);
                    }
                }
            }
            else if (((locFirst + idx) >= prefix) || 
                      ((tsk->loc[(int)tsk->subchan] <= (locFirst + idx + 3)) &&
                       (tsk->stat[(int)tsk->subchan] > 0) && (tsk->stat[(int)tsk->subchan] <= 4))) {
                        
                if (*c < -tsk->thresh[0]) {
                    if ((tsk->loc[(int)tsk->subchan] <= (locFirst + idx + 3)) &&
                        (tsk->strength[(int)tsk->subchan] > 0) &&
                        (tsk->strength[(int)tsk->subchan] > (-s >> 1))) {
                        tsk->signFlip = 2;
                    }
                    tsk->stat[(int)tsk->subchan] = 1;
                    VTSS_D("\txc_search: Open  @ tap %d, strength = %d < -thresh = -%d\n", locFirst + idx, s, tsk->thresh[0]);
                }
                else if (*c > tsk->thresh[0]) {
                    if ((tsk->loc[(int)tsk->subchan] <= (locFirst + idx + 3)) &&
                        (tsk->strength[(int)tsk->subchan] < 0) &&
                        (-tsk->strength[(int)tsk->subchan] > (s >> 1))) {
                        tsk->signFlip = 2;
                    }
                    tsk->stat[(int)tsk->subchan] = 2;
                    VTSS_D("\txc_search: Short @ tap %d, strength = %d > +thresh = %d\n", locFirst + idx, s, tsk->thresh[0]);
                }
                else {
                    tsk->stat[(int)tsk->subchan] = 4;
                    VTSS_D("\txc_search: Anom. @ tap %d, strength = %d > thresh = %d\n", locFirst + idx, s, tsk->thresh[1]);
                }
                tsk->loc[(int)tsk->subchan] = locFirst + idx;
                tsk->strength[(int)tsk->subchan] = s;
            }
        }                       
        tsk->signFlip = tsk->signFlip - 1;
    }                           
}

static const int c51_code FFEinit4_7search[2][4] = {
    {  1,   3,  -3,  -1 }, 
    { 10,  30, -30, -10 }  
};


static void FFEinit4_7(vtss_veriphy_task_t c51_idata *tsk, i8 taps4_7select)
{
    i8 idx, max;
    




    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
        VTSS_D("In FFEinit4_7 PHY_1_GEN_DSP");
        max = 28;
        tr_raw_write(tsk->port, 0x0240, 0x1ff); 
    } else {
        max = 16;
        tr_raw_write(tsk->port, 0x0240, 0x100); 
    }

    for (idx = 0; idx < max; ++idx) {      
        tr_raw_write(tsk->port, 0x0200 | (idx << 1), 0);
        if (idx == 3)
            idx = 7;
    }
    for (idx = 0; idx < 4; ++idx) {
        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            tr_raw_write(tsk->port, 0x0200 | ((4+idx) << 1), ((long)FFEinit4_7search[(int)taps4_7select][(int)idx]) << 16);
        } else {
            tr_raw_write(tsk->port, 0x0200 | ((4+idx) << 1), FFEinit4_7search[(int)taps4_7select][(int)idx] << 9);
        }
    }
    
    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
        tr_raw_write(tsk->port, 0x0240, 0x0ff);        
    } else {
        tr_raw_write(tsk->port, 0x0240, 0);      
    }
}

u16 SaveMediaSelect;

vtss_rc vtss_phy_veriphy_task_start(vtss_port_no_t port_no, u8 mode)
{
    vtss_phy_port_state_t         *ps = &vtss_state->phy_state[port_no];
    vtss_veriphy_task_t c51_idata *tsk;
    vtss_phy_family_t             family;
    u16                           reg23;

    tsk = &ps->veriphy;
    family = port_family(port_no);

    if (family == VTSS_PHY_FAMILY_MUSTANG || \
        family == VTSS_PHY_FAMILY_COBRA   || \
        family == VTSS_PHY_FAMILY_QUATTRO || \
        family == VTSS_PHY_FAMILY_LUTON   || \
        family == VTSS_PHY_FAMILY_LUTON_E || \
        family == VTSS_PHY_FAMILY_LUTON24 || \
        family == VTSS_PHY_FAMILY_COOPER  || \
        family == VTSS_PHY_FAMILY_SPYDER  || \
        family == VTSS_PHY_FAMILY_ENZO) {
            tsk->task_state = (u8) ((vtss_veriphy_task_state_t) VERIPHY_STATE_INIT_0);
            tsk->port = port_no;
            tsk->flags = mode << 6;
            VTSS_D("VeriPHY algorithm initialized, Port=%d, State=%d\n", tsk->port, tsk->task_state);
            VTSS_MTIMER_START(&tsk->timeout, 1); 

            
            
            vtss_phy_rd(tsk->port, 23, &reg23);
            SaveMediaSelect = (reg23 >> 6) & 3;
            reg23 = (reg23 & 0xff3f) | 0x0080;
            vtss_phy_wr(tsk->port, 23, reg23);
    } else if (family == VTSS_PHY_FAMILY_LUTON26 || 
               family == VTSS_PHY_FAMILY_ATOM || 
               family == VTSS_PHY_FAMILY_TESLA) {
        
        
        if (ps->setup.mode != VTSS_PHY_MODE_POWER_DOWN) {
            tsk->task_state = (u8) ((vtss_veriphy_task_state_t) VERIPHY_STATE_INIT_0);
            tsk->port = port_no;
            
            
            vtss_phy_veriphy_running(tsk->port, TRUE, TRUE);
            
            
            VTSS_RC(atom_patch_suspend(port_no, TRUE)); 
            
            
            VTSS_I("Starting VeriPhY, port_no = %d", port_no);
            vtss_phy_page_ext(port_no);
            PHY_WR_PAGE(port_no, VTSS_PHY_VERIPHY_CTRL_REG1, VTSS_F_PHY_VERIPHY_CTRL_REG1_TRIGGER);
            vtss_phy_page_std(port_no);
        } 
        VTSS_MTIMER_START(&tsk->timeout, 60000); 
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_phy_veriphy(vtss_veriphy_task_t c51_idata *tsk)
{
    u8 idx=0;
    u32 tr_raw_data;
    vtss_phy_family_t family;
    u16 reg_val;
    vtss_phy_port_state_t  *ps = &vtss_state->phy_state[tsk->port];

    family = port_family(tsk->port);

    if (family == VTSS_PHY_FAMILY_ATOM || \
        family == VTSS_PHY_FAMILY_LUTON26 || 
        family == VTSS_PHY_FAMILY_TESLA) {
        vtss_phy_page_ext(tsk->port);
        PHY_RD_PAGE(tsk->port, VTSS_PHY_VERIPHY_CTRL_REG1, &reg_val);
        VTSS_D("VeriPhY Complete reg24 =0x%X, port = %d", reg_val, tsk->port);                
        if (ps->setup.mode == VTSS_PHY_MODE_POWER_DOWN) {
            VTSS_I("Bugzilla#6769 - Ports get stuck if running Verify at port that is disabled.");
            tsk->loc[0] = 0;
            tsk->loc[1] = 0;
            tsk->loc[2] = 0;
            tsk->loc[3] = 0;
            tsk->stat[3] = VTSS_VERIPHY_STATUS_OPEN;
            tsk->stat[2] = VTSS_VERIPHY_STATUS_OPEN;
            tsk->stat[1] = VTSS_VERIPHY_STATUS_OPEN;
            tsk->stat[0] = VTSS_VERIPHY_STATUS_OPEN;
            tsk->flags = 1 << 1; 
            vtss_phy_page_std(tsk->port);
            return VTSS_RC_OK; 
        } else if ((reg_val & VTSS_F_PHY_VERIPHY_CTRL_REG1_TRIGGER) == 0x0) {
            
            tsk->loc[0] = ((reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG1_PAIR_A_DISTANCE) >> 8) * 3;
            tsk->loc[1] = (reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG1_PAIR_B_DISTANCE) * 3;
            
            PHY_RD_PAGE(tsk->port, VTSS_PHY_VERIPHY_CTRL_REG2, &reg_val);
            VTSS_D("VeriPhY Complete reg25E1:0x%X, port = %d", reg_val, tsk->port);                
            tsk->loc[2] = ((reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG2_PAIR_C_DISTANCE) >> 8) * 3;
            tsk->loc[3] = (reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG2_PAIR_D_DISTANCE) * 3;

            PHY_RD_PAGE(tsk->port, VTSS_PHY_VERIPHY_CTRL_REG3, &reg_val);
            VTSS_D("VeriPhY Complete reg26E1:0x%X, port = %d", reg_val, tsk->port);                
            tsk->stat[3] =  reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_D_TERMINATION_STATUS;
            tsk->stat[2] = (reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_C_TERMINATION_STATUS) >> 4;
            tsk->stat[1] = (reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_B_TERMINATION_STATUS) >> 8;
            tsk->stat[0] = (reg_val & VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_A_TERMINATION_STATUS) >> 12;
            
            tsk->flags = 1 << 1; 
            
            VTSS_I("VeriPhY Complete reg26E1:0x%X, port:%d", reg_val, tsk->port);                

            vtss_phy_page_std(tsk->port);

            
            vtss_phy_veriphy_running(tsk->port, TRUE, FALSE);

            
            VTSS_RC(atom_patch_suspend(tsk->port, FALSE)); 

            return VTSS_RC_OK; 
        } else if (VTSS_MTIMER_TIMEOUT(&tsk->timeout)) {
            vtss_phy_page_std(tsk->port);
            return VTSS_RC_ERROR;
        } else {
            VTSS_N("VeriPhY Running, port:%d, reg:0x%X", tsk->port, reg_val);
            vtss_phy_page_std(tsk->port);
            return VTSS_RC_INCOMPLETE; 
        }
    }


    if (!VTSS_MTIMER_TIMEOUT(&tsk->timeout))
        return VTSS_RC_INCOMPLETE;

    VTSS_D("---> ENTER VeriPHY state of port %d is: 0x%02x\n", tsk->port, tsk->task_state);
    VTSS_D("---> statA[%d], statB[%d], statC[%d], statD[%d] ", tsk->stat[0], tsk->stat[1], tsk->stat[2], tsk->stat[3]);

    
    if (tsk->task_state & VTSS_VERIPHY_STATE_DONEBIT) {
        VTSS_D("Handle cleanup for VeriPHY task abort");
        switch (tsk->task_state & ~VTSS_VERIPHY_STATE_DONEBIT) {

        case VTSS_VERIPHY_STATE_IDLE : 
            break;

        default :
            

            
            

            vtss_phy_page_tr(tsk->port);
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                tr_raw_write(tsk->port, 0x0b4, 0x1e8ed);
            } else {
                tr_raw_write(tsk->port, 0x0190, 0xf47);
            }

            if (family == VTSS_PHY_FAMILY_QUATTRO) {
                
                
                vtss_phy_page_tr(tsk->port);
                tr_raw_write(tsk->port, 0x0400, 0x1dec00);
            }

            
            if ((tsk->flags & 1) == 0)
            {
                
                if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                    vtss_phy_1_gen_post_veriphy(family, tsk);
                } else {
                    
                    vtss_phy_page_tr(tsk->port);
                    tr_raw_read(tsk->port, 0x0f86, &tr_raw_data);
                    tr_raw_data &= ~0x300000;
                    tr_raw_write(tsk->port, 0x0f86, tr_raw_data);
                }
            }

            if (family == VTSS_PHY_FAMILY_LUTON24) {
                
                ExtMiiWrite(tsk->port, 26, ((u16)tsk->stat[0] << 12) |
                            ((u16)(tsk->stat[1] & 0xf) << 8) |
                            ((u16)(tsk->stat[2] & 0xf) << 4) |
                            (u16)(tsk->stat[3] & 0xf) );
                ExtMiiWrite(tsk->port, 25, ((u16)(tsk->loc[2] & 0x3f) << 8) |
                            (u16)(tsk->loc[3] & 0x3f) |
                            ((u16)(tsk->flags & 1) << 15) );
                ExtMiiWrite(tsk->port, 24, ((u16)(tsk->loc[0] & 0x3f) << 8) |
                            (u16)(tsk->loc[1] & 0x3f) |
                            (u16)(tsk->flags & 0xc0) | 0x8000 );
            }

            if (family == VTSS_PHY_FAMILY_SPYDER || \
            	family == VTSS_PHY_FAMILY_ENZO || \
                family == VTSS_PHY_FAMILY_LUTON || \
                family == VTSS_PHY_FAMILY_LUTON_E || \
                family == VTSS_PHY_FAMILY_LUTON24 || \
                family == VTSS_PHY_FAMILY_COOPER) {
                
                TpWriteBit(tsk->port, 12, 0, 1);
            }

            
            if (tsk->flags & 1)
            {
                VTSS_MSLEEP(10);              

                if ((family == VTSS_PHY_FAMILY_MUSTANG) ||
                    (family == VTSS_PHY_FAMILY_COBRA) ||
                    (family == VTSS_PHY_FAMILY_COOPER)) {
                    
                    tsk->flags |= (MiiReadBits(tsk->port, 17, 13, 13) ^ 1) << 1;
                } else if (family == VTSS_PHY_FAMILY_LUTON || \
                           family == VTSS_PHY_FAMILY_LUTON_E || \
                           family == VTSS_PHY_FAMILY_SPYDER || \
                           family == VTSS_PHY_FAMILY_ENZO) {
                    tsk->flags |= ExtMiiReadBits(tsk->port, 24, 14, 14) << 1; 
                } else {
                    tsk->flags |= ExtMiiReadBits(tsk->port, 25, 14, 14) << 1; 
                }
            }
            else 
                tsk->flags |= 2; 

            if (family == VTSS_PHY_FAMILY_LUTON24) {
                
                MiiWriteBits(tsk->port, 23, 7, 6, tsk->flags2 & 3);
            }

            if (family == VTSS_PHY_FAMILY_LUTON ||
                family == VTSS_PHY_FAMILY_LUTON_E) {
                
                MiiWriteBits(tsk->port, 28, 6, 6, tsk->flags2 & 1);
            }

            if (family == VTSS_PHY_FAMILY_COOPER) {
                
                MiiWriteBits(tsk->port, 23, 5, 5, tsk->flags2 & 1);
            }

            
            TpWriteBit(tsk->port, 8, 9, 0);

            
            vtss_phy_page_std(tsk->port);
        }   

        {
            u16 reg23;
            
            vtss_phy_rd(tsk->port, 23, &reg23);
            reg23 = (reg23 & 0xff3f) | (SaveMediaSelect << 6);
            vtss_phy_wr(tsk->port, 23, reg23);
        }

        
        tsk->task_state = VTSS_VERIPHY_STATE_IDLE;
        return VTSS_RC_OK;
    }  

    switch (tsk->task_state)
    {
    
    case VTSS_VERIPHY_STATE_IDLE :
        VTSS_D("VTSS_VERIPHY_STATE_IDLE");
        if (family == VTSS_PHY_FAMILY_LUTON24) {
            for (idx = 0; idx < 8; ++idx) {
                
                tsk->port = (tsk->port + 1) & 7;
                vtss_phy_page_ext(tsk->port);
                tsk->thresh[0] = SmiRead(tsk->port, 24);
                if (tsk->thresh[0] & 0x8000)
                {
                    vtss_phy_page_std(tsk->port);
                    tsk->flags = (unsigned char)(tsk->thresh[0] & 0xc0); 
                    tsk->thresh[0] = SmiRead(tsk->port, 23);
                    tsk->flags2 = (tsk->thresh[0] >> 6) & 3; 
                    tsk->thresh[0] = (tsk->thresh[0] & 0xff3f) | 0x0080; 
                    vtss_phy_page_std(tsk->port);
                    SmiWrite(tsk->port, 23, tsk->thresh[0]);

                    tsk->task_state = VERIPHY_STATE_INIT_0;
                    break;
                }
            }
        }
        break;

    
    case VERIPHY_STATE_INIT_0 :
        
        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            tsk->numCoeffs = 12;
        } else {
            tsk->numCoeffs = 8;
        }
        for (idx = 0; idx < 4; ++idx) {
            tsk->stat[idx]     = 0;
            tsk->loc[idx]      = 0;
            tsk->strength[idx] = 0;
        }

        VTSS_D("VeriPHY init - TpWriteBit(%d, 8, 9, 1)", tsk->port);
        TpWriteBit(tsk->port, 8, 9, 1);   

        if (family == VTSS_PHY_FAMILY_QUATTRO) {
            
            
            VTSS_D("VeriPHY init - TrRawWrite(%d, 0, 8, 0x00, 0x1de800)", tsk->port);
            vtss_phy_page_tr(tsk->port);
            tr_raw_write(tsk->port, 0x0400, 0x1de800);
        }

        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            
            
            tsk->flags |= MiiReadBits(tsk->port, 17, 12, 12);
            if (tsk->flags & 1) 
            {
                ;
            }
            else 
            {
                vtss_phy_1_gen_pre_veriphy (family, tsk);
            }
        } else {
            VTSS_D("VeriPHY init - ExtMiiWrite(%d, 24, 0x8000)", tsk->port);
            ExtMiiWrite(tsk->port, 24, 0x8000);       
        }

        if (family == VTSS_PHY_FAMILY_LUTON24) {
            
            vtss_phy_page_std(tsk->port);
            tsk->thresh[0] = SmiRead(tsk->port, 23);
            tsk->flags2 = (tsk->thresh[0] >> 6) & 3;
            tsk->thresh[0] = (tsk->thresh[0] & 0xff3f) | 0x0080;
            vtss_phy_page_std(tsk->port);
            SmiWrite(tsk->port, 23, tsk->thresh[0]);
        }

        if (family == VTSS_PHY_FAMILY_LUTON ||
            family == VTSS_PHY_FAMILY_LUTON_E) {
            
            vtss_phy_page_std(tsk->port);
            tsk->thresh[0] = SmiRead(tsk->port, 28);
            tsk->flags2 = (tsk->thresh[0] >> 6) & 1;
            tsk->thresh[0] = tsk->thresh[0] & 0xffbf;
            vtss_phy_page_std(tsk->port);
            SmiWrite(tsk->port, 28, tsk->thresh[0]);
        }

        
        if (family == VTSS_PHY_FAMILY_COOPER) {
            
            vtss_phy_page_std(tsk->port);
            tsk->thresh[0] = SmiRead(tsk->port, 23);
            tsk->flags2 = (tsk->thresh[0] >> 5) & 1;
            tsk->thresh[0] = tsk->thresh[0] & 0xffdf;
            vtss_phy_page_std(tsk->port);
            SmiWrite(tsk->port, 23, tsk->thresh[0]);
        }

        
        if (!VTSS_PHY_1_GEN_DSP(tsk->port)) {
            
            
            VTSS_MTIMER_START(&tsk->timeout,750);
        }
        tsk->task_state = VERIPHY_STATE_INIT_1;
        break;

    
    case VERIPHY_STATE_INIT_1 :
        
        if (!VTSS_PHY_1_GEN_DSP(tsk->port)) {
            tsk->flags |= MiiReadBits(tsk->port, 17, 12, 12);
            VTSS_D("VeriPHY init - vphy_linkup = %d", tsk->flags & 1);
            if (ExtMiiReadBits(tsk->port, 24, 15, 15) == 0) {
                
                VTSS_D("VeriPHY Init failed: ExtMiiReadBits(tsk->port, 24, 15, 15) == 0");
                tsk->task_state |= 0x80; 
                break;
            }
        }
        if (tsk->flags & 1) {
            i8 vgaGain;

            
            
            vtss_phy_page_tr(tsk->port);
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                tr_raw_write(tsk->port, 0x0b4, 0x40ed);
            } else {
                tr_raw_write(tsk->port, 0x0190, 0x000387);
            }

            
            tr_raw_read(tsk->port, 0x0ff0, &tr_raw_data);
            vgaGain = (i8)(tr_raw_data >> 4) & 0x0f;

            if (tr_raw_data & 0x100) {
                vgaGain -= 16;
            }
            tsk->log2VGAx256 = 900 + (26 * (int)vgaGain);

            VTSS_D("VeriPHY link-up anomalySearch");
            tsk->task_state = VERIPHY_STATE_INIT_ANOMSEARCH_0;
        }
        else {
            tsk->thresh[0]   = 400; 
            VTSS_MTIMER_START(&tsk->timeout, 10); 
            tsk->task_state  = VERIPHY_STATE_INIT_LINKDOWN;
        }
        break;

    
    case VERIPHY_STATE_INIT_LINKDOWN :
        
        if (MiiReadBits(tsk->port, 28, 4, 3) == 2) {
            
            vtss_phy_page_tr(tsk->port);
            FFEinit4_7(tsk, FFEinit4_7anomSearch);

            
            
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                tr_raw_write(tsk->port, 0x0b4, 0x400d);
            } else {
                tr_raw_write(tsk->port, 0x0190, 0x381);
            }
            tr_raw_read(tsk->port, 0x0f86, &tr_raw_data);    
            tr_raw_data |= 0x300000;
            tr_raw_write(tsk->port, 0x0f86, tr_raw_data);
            tsk->log2VGAx256 = 0;

            VTSS_D("VeriPHY link-down anomalySearch");
            tsk->task_state = VERIPHY_STATE_INIT_ANOMSEARCH_0;
        }
        else if (--(tsk->thresh[0]) > 0) {
            VTSS_MTIMER_START(&tsk->timeout, 5); 
        }
        else { 
            tsk->task_state |= 0x80; 
        }
        break;

    
    case VERIPHY_STATE_INIT_ANOMSEARCH_0 :
        tsk->nc = (tsk->flags & 0x80) ? 0 : 3;
        tsk->thresh[2] = 0; 
    case VERIPHY_STATE_INIT_ANOMSEARCH_1 :
        tsk->thresh[1] = 0; 
        tsk->thresh[0] = 0; 
        

    
    
    case VERIPHY_STATE_ANOMSEARCH_0 :
        
        tsk->thresh[1] = tsk->thresh[0];

        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            
            
            tsk->tr_raw0188 = 0;
            
            if (tsk->nc == 0)
                VTSS_MTIMER_START(&tsk->timeout, 500);
        } else {
            vtss_phy_page_tr(tsk->port);
            tr_raw_write(tsk->port, 0x0184, 1); 

            
            tsk->tr_raw0188 = (long)tsk->nc * 0x0aa000;

            tr_raw_write(tsk->port, 0x0188, tsk->tr_raw0188);
            tr_raw_write(tsk->port, 0x0184, 0); 
            VTSS_D("VeriPHY delay 500ms");

            
            VTSS_MTIMER_START(&tsk->timeout, 500);
        }
        tsk->task_state = VERIPHY_STATE_ANOMSEARCH_1;
        break;

    
    case VERIPHY_STATE_ANOMSEARCH_1 :
        vtss_phy_page_tr(tsk->port);
        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            tr_raw_read(tsk->port, 0x0ba, &tr_raw_data);
            if (((tr_raw_data & 0xff0080) == 0xf00080))
            {
                
                tsk->thresh[0] = tsk->thresh[1] + 1;

                VTSS_D("VeriPHY %d%c: %s, blip is ZERO at all delays!", tsk->port, idx+'A', (tsk->nc>=2) ? ((tsk->nc > 2) ? "NC3":"NC2") : ((tsk->nc > 0) ? "NC1":"EC"));
                
                for (idx = 0; idx < 4;++idx) {
                    tsk->stat[idx]      = 0;
                    tsk->loc[idx]       = 0;
                    tsk->strength[idx]  = 0;
                }
            }
        } else {
            for (idx = 0; idx < 4; ++idx) {
                tr_raw_read(tsk->port, (idx << 11) | 0x01c0, &tr_raw_data);
                if (((tr_raw_data & 0xff0080) == 0xf00080)) {
                    
                    tsk->thresh[0] = tsk->thresh[1] + 1;

                    VTSS_D("VeriPHY %d%c: %s, blip is ZERO at all delays!", tsk->port, idx+'A', (tsk->nc>=2) ? ((tsk->nc > 2) ? "NC3":"NC2") : ((tsk->nc > 0) ? "NC1":"EC"));
                    
                    tsk->stat[idx]      = 0;
                    tsk->loc[idx]       = 0;
                    tsk->strength[idx]  = 0;
                }
            }
        }

        
        if (tsk->thresh[0] != 0)
        {
            
            if (tsk->thresh[0] == tsk->thresh[1])
            {
                
                if (tsk->nc == ((tsk->flags & 0x80) ? 0 : 3))
                {
                    
                    tsk->thresh[2] += tsk->thresh[0] - 1;

                    
                    VTSS_MTIMER_START(&tsk->timeout, 500); 
                    tsk->subchan     = 0;     
                    tsk->task_state  = VERIPHY_STATE_ANOMSEARCH_2;
                }
                else 
                {
                    
                    tsk->nc = (tsk->flags & 0x80) ? 0 : 3;

                    
                    tsk->thresh[0] = tsk->thresh[1] + 1;
                    VTSS_D("VeriPHY %d: After blip is ZERO at all delays, reset to NC3!", tsk->port);

                    
                    VTSS_MTIMER_START(&tsk->timeout, 500); 
                    tsk->task_state  = VERIPHY_STATE_ANOMSEARCH_0;
                }
            }
            
            
            else if ((tsk->thresh[2] + tsk->thresh[0]) < 10)
            {
                
                tsk->task_state = VERIPHY_STATE_ANOMSEARCH_0;
            }
            else 
            {
                tsk->task_state |= 0x80; 
            }
            if ((tsk->flags & 1) == 0) 
                FFEinit4_7(tsk, FFEinit4_7anomSearch);
        }
        else 
        {
            
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                VTSS_MTIMER_START(&tsk->timeout, 200);
            } else {
                if (tsk->nc == 0)
                    VTSS_MTIMER_START(&tsk->timeout, 200);
            }
            tsk->subchan     = 0;     
            tsk->task_state  = VERIPHY_STATE_ANOMSEARCH_2;
        }
        break;

    
    case VERIPHY_STATE_ANOMSEARCH_2 :
        
        tsk->stat[(int)tsk->subchan] = (tsk->stat[(int)tsk->subchan] << 4) | (tsk->stat[(int)tsk->subchan] & 0x0f);
        if (tsk->nc != 0 || (tsk->stat[(int)tsk->subchan] & 0xc0) != 0x80) {  
            tsk->signFlip = 0;
            vtss_phy_page_tr(tsk->port);

            
            if ((!VTSS_PHY_1_GEN_DSP(tsk->port)) || (tsk->nc == 0)) {
                
                if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                    VTSS_D("VERIPHY_STATE_ANOMSEARCH_2 PHY_1_GEN_DSP");
                    tr_raw_read(tsk->port, 0x00ba, &tr_raw_data);
                    VTSS_D("tr_raw_data (&0x00ba) = 0x%x\n", tr_raw_data);
                } else {
                    tr_raw_read(tsk->port, (tsk->subchan << 11) | 0x1c0, &tr_raw_data);
                }

                if (tr_raw_data & 128) {     
                    
                    
                    
                    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                        idx = 64 + (tr_raw_data >> 16); 
                    } else {
                        idx = ((tsk->nc > 0) ? 16 : 72) + (tr_raw_data >> 16);
                    }
                    VTSS_D("VeriPHY %d%c: %s, blip @ tap %d", tsk->port, tsk->subchan+'A', (tsk->nc>=2) ? ((tsk->nc > 2) ? "NC3":"NC2") : ((tsk->nc > 0) ? "NC1":"EC"), idx);
                    VTSS_D("possible anomaly near tap %d\n", idx);
                    get_anom_thresh(tsk, idx + (tsk->numCoeffs >> 1));
                    
                    tsk->firstCoeff = VTSS_PHY_1_GEN_DSP(tsk->port) ? 192 : 72;

                    xc_search(tsk, idx, 0);
                    if (tsk->stat[(int)tsk->subchan] > 0 && tsk->stat[(int)tsk->subchan] <= 0x0f) {
                        checkValidity(tsk, MAX_ABS_COEFF_ANOM_INVALID_NOISE);

                        
                        tsk->stat[(int)tsk->subchan] = (tsk->stat[(int)tsk->subchan] << 4) | (tsk->stat[(int)tsk->subchan] & 0x0f);
                    }
                } else {
                VTSS_D("VeriPHY %d%c: %s (no blip)", tsk->port, tsk->subchan+'A', (tsk->nc>=2) ? ((tsk->nc > 2) ? "NC3":"NC2") : ((tsk->nc > 0) ? "NC1":"EC"));
                }
            }
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                tsk->firstCoeff = (tsk->nc > 0) ? ((tsk->nc << 5) - 20) : 0xb4;
            } else {
                tsk->firstCoeff = (tsk->nc > 0) ? ((tsk->nc << 4) + 72) : 64;
            }
            tsk->task_state = VERIPHY_STATE_ANOMSEARCH_3;
        }
        else if (tsk->subchan < 3) { 
            (tsk->subchan)++; 
        }
        else if (tsk->nc > 0) { 
            (tsk->nc)--;
            tsk->task_state = VERIPHY_STATE_INIT_ANOMSEARCH_1;
        }
        else { 
            tsk->task_state = VERIPHY_STATE_INIT_CABLELEN;
            tsk->subchan = 0;
        }
        break;

    
    case VERIPHY_STATE_ANOMSEARCH_3 :
        
        if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
            if (tsk->firstCoeff >= 128) 
                idx = tsk->firstCoeff - 128;
            else 
                idx = tsk->firstCoeff & 0x1f;
        } else {
            if (tsk->firstCoeff < 80) 
                idx = tsk->firstCoeff;
            else 
                idx = tsk->firstCoeff & 0x0f;
        }
        get_anom_thresh(tsk, idx + (tsk->numCoeffs >> 1));
        xc_search(tsk, idx, (idx == 4) ? 6 : 0);
        if (tsk->stat[(int)tsk->subchan] > 0 && tsk->stat[(int)tsk->subchan] <= 0x0f) {
            checkValidity(tsk, MAX_ABS_COEFF_ANOM_INVALID_NOISE);

            
            tsk->stat[(int)tsk->subchan] = (tsk->stat[(int)tsk->subchan] << 4) | (tsk->stat[(int)tsk->subchan] & 0x0f);
        }
        if (tsk->nc > 0) {
            if (idx > 0)
                tsk->firstCoeff -= tsk->numCoeffs;
            else
            {
                
                if ((tsk->stat[(int)tsk->subchan] & 0x0f) > 0 && (tsk->stat[(int)tsk->subchan] & 0x08) == 0) {
                    tsk->stat[(int)tsk->subchan] = (tsk->stat[(int)tsk->subchan] & 0xf4) | 8;
                    if (tsk->nc != tsk->subchan)
                        tsk->stat[(int)tsk->subchan] += tsk->nc;
                }
            }
        }
        else { 
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                if (idx > 16)
                    tsk->firstCoeff -= tsk->numCoeffs;
                else if (idx == 16) {
                    tsk->firstCoeff  = 0x80 | 5;
                    tsk->numCoeffs = 11;
                }
                else {
                    tsk->numCoeffs = 12; 
                    idx = 0; 
                }
            } else {
                if (idx > 8)
                    tsk->firstCoeff -= tsk->numCoeffs;
                else if (idx == 8) {
                    tsk->firstCoeff  = 4;
                    tsk->numCoeffs = 4;
                }
                else {
                    tsk->numCoeffs = 8; 
                    idx = 0; 
                }
            }
        }
        if (idx == 0) {
            if (tsk->subchan < 3) { 
                (tsk->subchan)++;
                tsk->task_state = VERIPHY_STATE_ANOMSEARCH_2;
            }
            else if (tsk->nc > 0) { 
                (tsk->nc)--;
                tsk->task_state = VERIPHY_STATE_INIT_ANOMSEARCH_1;
            }
            else { 
                tsk->task_state = VERIPHY_STATE_INIT_CABLELEN;
                tsk->subchan = 0;
            }
        }
        break;

    
    case VERIPHY_STATE_INIT_CABLELEN :
        for (idx = 0; (tsk->flags & 0xc0) == 0 && idx < 4; ++idx)
        {
            tsk->stat[idx] &= 0x0f;
            if (tsk->stat[idx] == 0) {
                if ((tsk->flags & 0xc0) != 0)
                    tsk->loc[idx] = 255; 
                else
                    tsk->flags |= 4;
            }
        }
        if ((tsk->flags & 0xc0) != 0 || (tsk->flags & 4) == 0) 
            tsk->task_state = VERIPHY_STATE_PAIRSWAP;
        else 
        {
            vtss_phy_page_tr(tsk->port);
            VTSS_D("VeriPHY getCableLength");
            if ((tsk->flags & 1) == 0) {
                FFEinit4_7(tsk, FFEinit4_7lengthSearch);
               
                
                if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                    tr_raw_write(tsk->port, 0x0b4, 0x96ad);
                } else {
                    tr_raw_write(tsk->port, 0x190, 0x4b5);
                }
            }
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                
                tr_raw_write(tsk->port, 0x0b6, (((232-64) << 2) | 3));
                tr_raw_write(tsk->port, 0x0b6, (((232-64) << 2) | 2)); 
            } else {
                
                tr_raw_write(tsk->port, 0x0188, ((232 - 72) << 1) | 1);
                tr_raw_write(tsk->port, 0x0184, 1);   
                tr_raw_write(tsk->port, 0x0184, 0);   
            }
            idx = 0xff;
            for (tsk->subchan = 0; tsk->subchan < 4; ++(tsk->subchan))
            {
                if (tsk->stat[(int)tsk->subchan] != 0)
                    tsk->thresh[(int)tsk->subchan] = 0;
                else
                {
                    for (tsk->signFlip = 0, maxAbsCoeff = 0; maxAbsCoeff < ((tsk->flags & 1) ? 1 : 4) && tsk->signFlip < 100; ++(tsk->signFlip))
                    {
                        

                        readAvgECNCECVar((tsk->subchan << 6) | tsk->port,
                                         family == VTSS_PHY_1_GEN_DSP(tsk->port) ? 192 : 72,
                                         0xf8);

                        if (maxAbsCoeff < ((tsk->flags & 1) ? 1 : 4)) {
                            VTSS_MSLEEP(2);
                        }
                        else
                            tsk->thresh[(int)tsk->subchan] = maxAbsCoeff;
                    }
                    if (tsk->signFlip > 1)
                    {
                        VTSS_D("VeriPHY %d%c: maxAbsC(tsk->thresh) = %d took %d attempts!", tsk->port, tsk->subchan+'A', tsk->thresh[(int)tsk->subchan], tsk->signFlip);
                    }
                    if (tsk->thresh[(int)tsk->subchan] < 14)
                        tsk->thresh[(int)tsk->subchan] = 20;
                    else
                        tsk->thresh[(int)tsk->subchan] += 6;
                    if (tsk->thresh[(int)tsk->subchan] > MAX_ABS_COEFF_LEN_INVALID_NOISE)
                    {
                        VTSS_D("VeriPHY %d%c: maxAbsC(tsk->thresh) = %d > noise limit of %d!", tsk->port, tsk->subchan+'A', tsk->thresh[(int)tsk->subchan], MAX_ABS_COEFF_LEN_INVALID_NOISE);
                        tsk->loc[(int)tsk->subchan] = 255;
                        tsk->strength[(int)tsk->subchan] = tsk->thresh[(int)tsk->subchan];
                    }
                    else if (idx == 0xff)
                        idx = (tsk->subchan << 2) | tsk->subchan;
                    else if (tsk->thresh[(int)tsk->subchan] > tsk->thresh[idx >> 2])
                        idx = tsk->subchan << 2 | (idx & 3);
                    if (idx != 0xff && tsk->thresh[(int)tsk->subchan] < tsk->thresh[idx & 3])
                        idx = (idx & 0x0c) | tsk->subchan;
                }
            }
            
            if (idx != 0xff && (tsk->thresh[idx >> 2] >= (tsk->thresh[idx & 3] + (tsk->thresh[idx & 3] >> 1))))
                tsk->flags = ((idx << 2) & 0x30) | 0x08 | (tsk->flags & 0xc7);
            else
                tsk->flags = (tsk->flags & 0xc7);

            VTSS_D("VeriPHY %d: tsk->thresh = { %d, %d, %d, %d }, unreliablePtr = %d", tsk->port, tsk->thresh[0], tsk->thresh[1], tsk->thresh[2], tsk->thresh[3], (((tsk->flags >> 4) & 3) + (tsk->flags & 8)));

            tsk->flags &= ~4; 
            tsk->signFlip = 0;
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                tsk->tr_raw0188 = (168 << 2) | 2;
                tsk->firstCoeff = 192;
            } else {
                tsk->tr_raw0188 = (160 << 1) | 1;
                tsk->firstCoeff = 72;
            }
            tsk->task_state = VERIPHY_STATE_GETCABLELEN_0;
        }
        break;

    
    case VERIPHY_STATE_GETCABLELEN_0 :
        if (((tsk->flags & 4) == 0) && ((tsk->firstCoeff & 0x7f) >= tsk->numCoeffs))
        {
            
            if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                if (tsk->firstCoeff == 192) {
                    idx = tsk->tr_raw0188 >> 2;
                    if (idx > 0) {
                        
                        idx -= tsk->numCoeffs;
                        tsk->tr_raw0188 = (idx << 2) | 2;
                        idx += 64; 
                    }
                    else { 
                        
                        tsk->tr_raw0188 = 0;
                        tsk->firstCoeff = 0x80 | 52;

                        
                        for (idx = 0; idx < 4; ++idx) {
                            tsk->thresh[idx] = tsk->thresh[idx] + (tsk->thresh[idx] >> 2);
                        }
                        idx = tsk->firstCoeff & 0x3f; 
                    }

                    vtss_phy_page_tr(tsk->port);
                    tr_raw_write(tsk->port, 0x0b6, tsk->tr_raw0188 | 1); 
                    tr_raw_write(tsk->port, 0x0b6, tsk->tr_raw0188);     
                }
                else {
                    tsk->firstCoeff -= tsk->numCoeffs;
                    idx = tsk->firstCoeff & 0x3f; 
                }
            } else {
                if (tsk->firstCoeff == 72) {
                    idx = tsk->tr_raw0188 >> 1;
                    if (idx > 0) {
                        
                        idx -= tsk->numCoeffs;
                        tsk->tr_raw0188 = (idx << 1) | 1;
                        idx += 72; 
                    }
                    else { 
                        
                        tsk->tr_raw0188 = 0;
                        tsk->firstCoeff = 64;

                        
                        for (idx = 0; idx < 4; ++idx) {
                            tsk->thresh[idx] = tsk->thresh[idx] + (tsk->thresh[idx] >> 2);
                        }
                        idx = tsk->firstCoeff; 
                    }

                    vtss_phy_page_tr(tsk->port);
                    tr_raw_write(tsk->port, 0x0188, tsk->tr_raw0188);
                    tr_raw_write(tsk->port, 0x0184, 1);     
                    tr_raw_write(tsk->port, 0x0184, 0);     
                }
                else {
                    tsk->firstCoeff -= tsk->numCoeffs;
                    idx = tsk->firstCoeff; 
                }
            }

           VTSS_D("probing for cable length at tap %d\n", idx);
            
            if ((tsk->loc[0] > 0) && (tsk->loc[1] > 0) && (tsk->loc[2] > 0) && (tsk->loc[3] > 0))
                tsk->flags |= 4; 

            for (tsk->subchan = 0; tsk->subchan < 4; ++(tsk->subchan)) {
                
                vtss_phy_page_tr(tsk->port);
                if (tsk->stat[(int)tsk->subchan] == 0 && ((idx+12) > tsk->loc[(int)tsk->subchan])) {
                    short c51_idata *c = readAvgECNCECVar((tsk->subchan << 6) | tsk->port,
                                                          tsk->firstCoeff, 0x20 | tsk->numCoeffs);
                    u8 cnt;

                    c   += tsk->numCoeffs;
                    idx += tsk->numCoeffs;
                    cnt  = tsk->numCoeffs;
                    do {
                        --idx;
                        --c;
                        if (((tsk->signFlip >> tsk->subchan) & 1) != 0) {
                            if (tsk->loc[(int)tsk->subchan] == (idx + 1)) {
                                if ( (tsk->strength[(int)tsk->subchan] < 0 && *c < tsk->strength[(int)tsk->subchan]) ||
                                     (tsk->strength[(int)tsk->subchan] > 0 && *c > tsk->strength[(int)tsk->subchan]) ) {
                                    tsk->loc[(int)tsk->subchan] = idx;
                                    tsk->strength[(int)tsk->subchan] = *c;
                                    VTSS_D("VeriPHY %d%c: ", (int)tsk->port, (int)(tsk->subchan+'A'));
                                    VTSS_D("sign-move 2 tap %d, strength = %d\n", (int)tsk->loc[(int)tsk->subchan], tsk->strength[(int)tsk->subchan]);
                                }
                            }
                        }
                        else if (ABS(*c) > tsk->thresh[(int)tsk->subchan]) {
                            if ((tsk->strength[(int)tsk->subchan] == 0) ||
                                ((tsk->loc[(int)tsk->subchan] <= (idx + 3)) &&
                                 (((tsk->strength[(int)tsk->subchan] > 0) &&
                                   (tsk->strength[(int)tsk->subchan] <= *c))
                                  || ((tsk->strength[(int)tsk->subchan] < 0) &&
                                      (tsk->strength[(int)tsk->subchan] >= *c))
                                 ))) {
                                if (tsk->strength[(int)tsk->subchan] == 0) { 
                                    tsk->strength[(int)tsk->subchan] = *c;
                                    tsk->loc[(int)tsk->subchan] = idx;
                                    checkValidity(tsk, MAX_ABS_COEFF_LEN_INVALID_NOISE);
                                    if (tsk->strength[(int)tsk->subchan] != 0) {
                                        VTSS_D("VeriPHY %d%c: ", (int)tsk->port, (int)(tsk->subchan+'A'));
                                        VTSS_D("blip-det. @ tap %d, strength = %d\n", idx, *c);
                                    }
                                }
                                else {
                                    tsk->strength[(int)tsk->subchan] = *c;
                                    tsk->loc[(int)tsk->subchan] = idx;
                                    VTSS_D("VeriPHY %d%c: ", (int)tsk->port, (int)(tsk->subchan+'A'));
                                    VTSS_D("blip-move 2 tap %d, strength = %d\n", idx, *c);
                                }
                            }
                            else if ((tsk->loc[(int)tsk->subchan] <= (idx + 5)) &&
                                       (((tsk->strength[(int)tsk->subchan] > 0) &&
                                         ((-tsk->strength[(int)tsk->subchan] + (tsk->strength[(int)tsk->subchan]>>3)) >= *c))
                                     || ((tsk->strength[(int)tsk->subchan] < 0) &&
                                         ((-tsk->strength[(int)tsk->subchan] + (tsk->strength[(int)tsk->subchan]>>3)) <= *c))
                                       )) {
                                tsk->loc[(int)tsk->subchan] = idx;
                                tsk->strength[(int)tsk->subchan] = *c;
                                tsk->signFlip |= 1 << tsk->subchan;
                                VTSS_D("VeriPHY %d%c: ", (int)tsk->port, (int)(tsk->subchan+'A'));
                                VTSS_D("sign-flip @ tap %d, strength = %d\n", tsk->loc[(int)tsk->subchan], tsk->strength[(int)tsk->subchan]);
                            }
                        }
                    } while (--cnt != 0);
                }
            }                   
            if (tsk->flags & 4) {
                tsk->task_state = VERIPHY_STATE_GETCABLELEN_1;
                vtss_phy_page_tr(tsk->port);
                
                if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                    tr_raw_write(tsk->port, 0x0b6, 1); 
                    tr_raw_write(tsk->port, 0x0b6, 0); 
                } else {
                    tr_raw_write(tsk->port, 0x0188, 0); 
                    tr_raw_write(tsk->port, 0x0184, 1); 
                    tr_raw_write(tsk->port, 0x0184, 0); 
                }
            }
        }
        else
            tsk->task_state = VERIPHY_STATE_GETCABLELEN_1;
        break;

    
    case VERIPHY_STATE_GETCABLELEN_1 :
        
        {
        u8 maxptr, minptr, max2ptr, min2ptr, endloc = 0;

        maxptr = minptr = 8;
        for (idx = 0; idx < 4; ++idx)
        {
            if (tsk->stat[idx] == 0 && tsk->loc[idx] < 255)
            {
                short s;

                get_anom_thresh(tsk, tsk->loc[idx]);
                if (tsk->loc[idx] <= 8)
                    s = 0;                  
                else if (tsk->flags & 1)    
                    s = (3*tsk->strength[idx] + 2) >> 2;  
                else 
                    s = (3*tsk->strength[idx] + 16) >> 5; 

                if (s > tsk->thresh[0]) {
                    tsk->stat[idx] = 2;
                    VTSS_D("VeriPHY %d%c: changing length (%d) to short (%d > %d)!\n", tsk->port, idx+'A', endloc, s, tsk->thresh[0]);
                }
                else if (s < -tsk->thresh[0]) {
                    tsk->stat[idx] = 1;
                    VTSS_D("VeriPHY %d%c: changing length (%d) to open (%d < -%d)!\n", tsk->port, idx+'A', endloc, s, tsk->thresh[0]);
                }
                else if (ABS(s) > tsk->thresh[1]) {
                    tsk->stat[idx] = 4;
                    VTSS_D("VeriPHY %d%c: changing length (%d) to anom (ABS(%d) > %d)!\n", tsk->port, idx+'A', endloc, s, tsk->thresh[1]);
                }
                else if (((tsk->signFlip >> idx) & 1) == 0) {   
                    
                    if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                        tsk->loc[idx] = tsk->loc[idx] - 3;      
                    } else {
                        tsk->loc[idx] = tsk->loc[idx] - 1;      
                    }
                }
                VTSS_D("subchan %c, loc = %d\n", idx+'A', tsk->loc[idx]);

                if (((tsk->flags & 8) == 0 || idx != ((tsk->flags >> 4) & 3)) && ((minptr >= 4) || (tsk->loc[idx] < tsk->loc[minptr])))
                    minptr = idx;
                if (((tsk->flags & 8) == 0 || idx != ((tsk->flags >> 4) & 3)) && ((maxptr >= 4) || (tsk->loc[idx] > tsk->loc[maxptr])))
                    maxptr = idx;
            }
        }
        if (minptr == 8) {
            
            if (tsk->flags & 8) {
                endloc = tsk->loc[(tsk->flags >> 4) & 3];
                VTSS_D("VeriPHY %d: unreliable endloc = %d is only available!\n", tsk->port, endloc);
            }
            else {
                endloc = 255;
                VTSS_D("VeriPHY %d: No length (endloc = 255) is available!\n", tsk->port);
            }
        }
        else {
            
            if ((tsk->flags & 8) && tsk->loc[(tsk->flags >> 4) & 3] >= (tsk->loc[minptr] - 1)) {
                if (tsk->loc[(tsk->flags >> 4) & 3] == (tsk->loc[minptr] - 1)) {
                    minptr = (tsk->flags >> 4) & 3;
                    tsk->flags &= ~8;
                }
                else if (tsk->loc[(tsk->flags >> 4) & 3] <= (tsk->loc[maxptr] + 1)) {
                    if (tsk->loc[(tsk->flags >> 4) & 3] == (tsk->loc[maxptr] + 1))
                        maxptr = (tsk->flags >> 4) & 3;
                    tsk->flags &= ~8;
                }
            }

            
            min2ptr = maxptr;
            max2ptr = minptr;
            for (idx = 0; idx < 4; ++idx) {
                if (tsk->stat[idx] == 0 && tsk->loc[idx] < 255 && ((tsk->flags & 8) == 0 || idx != ((tsk->flags >> 4) & 3))) {
                    if ((idx != minptr) && (tsk->loc[idx] < tsk->loc[min2ptr]))
                        min2ptr = idx;
                    if ((idx != maxptr) && (tsk->loc[idx] > tsk->loc[max2ptr]))
                        max2ptr = idx;
                }
            }
            endloc = (max2ptr == minptr) ? tsk->loc[maxptr] : (u8)(((u32)tsk->loc[min2ptr] + tsk->loc[max2ptr]) >> 1);
            VTSS_D("VeriPHY %d: max/minptr = %d/%d, max2/min2ptr = %d/%d, endloc = %d!\n", tsk->port, maxptr, minptr, max2ptr, min2ptr, endloc);
        }
        for (idx = 0; idx < 4; ++idx) {
            if (tsk->stat[idx] == 0) {
                VTSS_D("VeriPHY loc[%d] = %d --> %d\n", idx, tsk->loc[idx], endloc);
                tsk->loc[idx] = endloc;
            }
        }
        tsk->task_state = VERIPHY_STATE_PAIRSWAP;
        }
        break;

    
    case VERIPHY_STATE_PAIRSWAP :
        
        
        VTSS_D("VeriPHY swapPairs");
        {
            i8 MDIX_CDpairSwap;
            u8 ubtemp;
            short stemp;

            
            MDIX_CDpairSwap = MiiReadBits( tsk->port, 28, 13, 12 );

            if (MDIX_CDpairSwap < 2) {
                
                stemp            = tsk->strength[1];
                tsk->strength[1] = tsk->strength[0];
                tsk->strength[0] = stemp;
                ubtemp           = tsk->stat[1];
                tsk->stat[1]     = tsk->stat[0];
                tsk->stat[0]     = ubtemp;
                ubtemp           = tsk->loc[1];
                tsk->loc[1]      = tsk->loc[0];
                tsk->loc[0]      = ubtemp;

                
                for (idx = 0; idx < 4; ++idx) {
                    if ( (tsk->stat[idx] & 10) == 8 )
                        tsk->stat[idx] = tsk->stat[idx] ^ 1;
                }
            }

            if ((MDIX_CDpairSwap == 0) || (MDIX_CDpairSwap == 3)) {
                
                stemp            = tsk->strength[3];
                tsk->strength[3] = tsk->strength[2];
                tsk->strength[2] = stemp;
                ubtemp           = tsk->stat[3];
                tsk->stat[3]     = tsk->stat[2];
                tsk->stat[2]     = ubtemp;
                ubtemp           = tsk->loc[3];
                tsk->loc[3]      = tsk->loc[2];
                tsk->loc[2]      = ubtemp;

                
                for (idx = 0; idx < 4; ++idx) {
                    if ( (tsk->stat[idx] & 10) == 10 )
                        tsk->stat[idx] = tsk->stat[idx] ^ 1;
                }
            }
        }

        
        for (idx = 0; idx < 4; ++idx) {
            u8 loc = tsk->loc[idx];
            VTSS_D("Tap Location - loc[%d] = %d\n", idx, tsk->loc[idx]);

            if (loc <= 7)
                tsk->loc[idx] = 0;
            else if (loc < 255)
            {
                
                
                
                if (VTSS_PHY_1_GEN_DSP(tsk->port)) {
                    if (tsk->flags & 1) {
                        if (family == VTSS_PHY_FAMILY_MUSTANG) {
                            loc -= 13;
                        } else
                            loc -= 7;
                    } else
                        loc -= 7;
                } else {
                    if (tsk->flags & 1)
                        loc -= 7;
                    else
                        loc -= 6;
                }
                loc = loc - ((((loc + (loc >> 4)) >> 2) + 1) >> 1);
                tsk->loc[idx] = loc;
                if ((tsk->stat[idx] == 2) && (loc < 5)) {
                    tsk->stat[idx] = 4;
                }
            }
        }
        tsk->task_state = VERIPHY_STATE_FINISH;
        break;
    }

    if (tsk->task_state == VTSS_VERIPHY_STATE_IDLE) { 
        VTSS_MTIMER_CANCEL(&tsk->timeout);
    }

    VTSS_D("<--- EXIT VeriPHY state of port %d is: 0x%02x\n", tsk->port, tsk->task_state);

    return (tsk->task_state == VTSS_VERIPHY_STATE_IDLE) ? VTSS_RC_OK : VTSS_RC_INCOMPLETE;
}


#endif 
