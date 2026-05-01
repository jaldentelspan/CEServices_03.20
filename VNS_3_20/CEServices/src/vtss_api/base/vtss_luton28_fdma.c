/*

 Vitesse Switch Software.

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

*/
/*****************************************************************************/
//
// vtss_luton28_fdma.c
//
//*****************************************************************************/

#define VTSS_IOADDR(t,o) (((VTSS_TB_ ## t + o) - VTSS_TB_SWC) >> 2)
#define VTSS_REGISTER(t,o) (VTSS_IOADDR(t,o))

#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_CIL

// Avoid "vtss_api.h not used in module vtss_luton28_fdma.c"
/*lint --e{766} */

#include "vtss_api.h"
#if defined(VTSS_ARCH_LUTON28) && VTSS_OPT_FDMA
#include "vtss_state.h"
#include "vtss_fdma.h"
#include "vtss_fdma_common.h"
#include "vtss_common.h" /* For vtss_cmn_XXX() functions */

#if defined(VTSS_OPSYS_ECOS)
#include <cyg/hal/vcoreii_amba_top.h>
#elif defined(VTSS_OPSYS_LINUX)
#include <asm/hardware.h>
#else
#error "Operating system not supported".
#endif /* defined(VTSS_OPSYS_xxx) */

#if VTSS_OPT_FDMA_VER >= 2
  // To cater for the VTSS_OPT_FDMA_VER >= 2 case, we need to define the GPDMA/FDMA-expected
  // header sizes so that the IFH starts at the correct position.
  #define VTSS_FDMA_XTR_HDR_SIZE_BYTES  8 // Number of bytes in the extraction header (equal to VTSS_FDMA_IFH_SIZE_BYTES).
  #define VTSS_FDMA_INJ_HDR_SIZE_BYTES 12 // Number of bytes in the injection header (equal to VTSS_FDMA_IFH_SIZE_BYTES + VTSS_FDMA_CMD_SIZE_BYTES).
#endif /* defined(VTSS_OPT_FDMA_VER >= 2 */

// We don't operate with assertions in the API, so we print an error and gracefully returns.
#define FDMA_ASSERT(grp, expr, code) do {if(!(expr)) {VTSS_EG((VTSS_TRACE_GROUP_FDMA_ ## grp), "Assert failed: " # expr); code}} while(0);

// Debug register and thus not in the datasheet
#define VTSS_MISC_DMA_COMP_VERSION VTSS_REGISTER(FDMA,0x03FC)
#define DW_DMAC_VER 0x3230372A

// Extraction Queue Registers. Located in the switch core.
#define VTSS_XTR_QU_RST(qu)                     VTSS_REGISTER(SWC, BLK_SUBBLK_ADDR(4, (2 * (qu)), 0x01))

// Registers that already are defined in amba_devices.h, but need to be re-defined,
// so that we can iterate over a variable
#define GPDMA_FDMA_CH_CFG_0(ch)                 VTSS_REGISTER(ICPU_CFG, (0x0060+(4*(ch))))
#define GPDMA_FDMA_CH_CFG_1(ch)                 VTSS_REGISTER(ICPU_CFG, (0x0080+(4*(ch))))
#define GPDMA_FDMA_INJ_CFG(inj_ch)              VTSS_REGISTER(ICPU_CFG, (0x00A0+(4*(inj_ch))))
#define GPDMA_FDMA_XTR_CFG(xtr_ch)              VTSS_REGISTER(ICPU_CFG, (0x00C0+(4*(xtr_ch))))
#define VTSS_CH_LLP(ch)                         VTSS_REGISTER(FDMA,     (0x0010+(4*22*(ch))))
#define VTSS_CH_CTL0(ch)                        VTSS_REGISTER(FDMA,     (0x0018+(4*22*(ch))))
#define VTSS_CH_DSTATAR(ch)                     VTSS_REGISTER(FDMA,     (0x0038+(4*22*(ch))))
#define VTSS_CH_CFG0(ch)                        VTSS_REGISTER(FDMA,     (0x0040+(4*22*(ch))))
#define VTSS_CH_CFG1(ch)                        VTSS_REGISTER(FDMA,     (0x0044+(4*22*(ch))))

// Injection Write Delay field definitions
#define VTSS_F_INJ_CFG_WR_DELAY_FPOS 1
#define VTSS_F_INJ_CFG_WR_DELAY_FLEN 5
#define VTSS_F_INJ_CFG_WR_DELAY(x)   VTSS_ENCODE_BITFIELD(x, VTSS_F_INJ_CFG_WR_DELAY_FPOS, VTSS_F_INJ_CFG_WR_DELAY_FLEN)

// This is needed to obtain the physical address of the CFG::XTR_LAST_CHUNK_STAT register
#define FDMA_LAST_CHUNK_PHYS_ADDR(ch)          (VTSS_TB_ICPU_CFG + 0x00e0 + 4*(ch))

// Debug fields not defined in amba_devices.h
// CFG::VTSS_GPDMA_FDMA_COMMON_CFG.DATA_BYTE_SWAP
#define VTSS_F_DATA_BYTE_SWAP VTSS_BIT(0)
// CFG::VTSS_CH_CFG1.PROTCTL
#define VTSS_F_PROTCTL(x)     VTSS_ENCODE_BITFIELD((x),2,3)

// The FDMA overrides the usage of the DCB->dar field in the injection case
#define FDMA_DAR_REMOTE(x)     VTSS_ENCODE_BITFIELD((x),21, 1)
#define FDMA_DAR_EOF(x)        VTSS_ENCODE_BITFIELD((x),20, 1)
#define FDMA_DAR_SOF(x)        VTSS_ENCODE_BITFIELD((x),19, 1)
#define FDMA_DAR_CH_ID(x)      VTSS_ENCODE_BITFIELD((x),16, 3)
#define FDMA_DAR_CHUNK_SIZE(x) VTSS_ENCODE_BITFIELD((x), 4,12)
#define FDMA_DAR_SAR_OFFSET(x) VTSS_ENCODE_BITFIELD((x), 2, 2)

#define FDMA_SAR_REMOTE(x)     VTSS_ENCODE_BITFIELD((x),21, 1)
#define FDMA_SAR_CH_ID(x)      VTSS_ENCODE_BITFIELD((x),16, 3)
#define FDMA_SAR_CHUNK_SIZE(x) VTSS_ENCODE_BITFIELD((x), 4,12)

#define FDMA_F_SAR_CH_ID_FPOS 16
#define FDMA_F_SAR_CH_ID_FLEN  3

#define FDMA_INLINE inline

/*****************************************************************************/
// Read target register using current CPU interface
/*****************************************************************************/
static FDMA_INLINE u32 VCOREII_RD(vtss_state_t *const vstate, u32 reg)
{
    u32 value;
    FDMA_ASSERT(IRQ, vstate->init_conf.reg_read(0, reg, &value) == VTSS_RC_OK, return 0;);
    return value;
}

/*****************************************************************************/
// Write target register using current CPU interface
/*****************************************************************************/
static FDMA_INLINE void VCOREII_WR(vtss_state_t *const vstate, u32 reg, u32 value)
{
    FDMA_ASSERT(IRQ, vstate->init_conf.reg_write(0, reg, value) == VTSS_RC_OK, ;);
}

// Position of various field in the CMD field
#define FDMA_CMD_F_FRAMEDEST_FPOS      4
#define FDMA_CMD_F_DOANALYZE_FPOS      3
#define FDMA_CMD_F_REWIND_CPU_TX_FPOS  1
#define FDMA_CMD_F_CPU_TX_FPOS         0

#define FDMA_CMD_F_FRAMEDEST_FLEN     28
#define FDMA_CMD_F_DOANALYZE_FLEN      1
#define FDMA_CMD_F_REWIND_CPU_TX_FLEN  1
#define FDMA_CMD_F_CPU_TX_FLEN         1

#if VTSS_OPT_FDMA_VER >= 3
// Channel utilization
#define FDMA_CH_TX_SWITCHED 0
#define FDMA_CH_TX_PORTSPEC 5
#define FDMA_CH_TX_STACK_A  6
#define FDMA_CH_TX_STACK_B  7
#endif /* VTSS_OPT_FDMA_VER >= 3 */

/*****************************************************************************/
// check_version()
// Returns: 0 on failure to support the current silicon, 1 on success.
/*****************************************************************************/
static FDMA_INLINE int check_version(vtss_state_t *const vstate)
{
    return VCOREII_RD(vstate, VTSS_MISC_DMA_COMP_VERSION) == DW_DMAC_VER;
}

/*****************************************************************************/
// enable_ch()
/*****************************************************************************/
static FDMA_INLINE void enable_ch(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    // Create a memory barrier here, so that the compiler doesn't enable the channel
    // before it's configured properly (needed because this function is inline).
    VTSS_OS_REORDER_BARRIER();
    VCOREII_WR(vstate, VTSS_MISC_CH_EN_REG, VTSS_F_CH_EN_WE(VTSS_BIT(ch)) | VTSS_F_CH_EN(VTSS_BIT(ch))); // CH_EN_WE=CH_EN=1
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// disable_ch()
/*****************************************************************************/
static FDMA_INLINE void disable_ch(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    VCOREII_WR(vstate, VTSS_MISC_CH_EN_REG, VTSS_F_CH_EN_WE(VTSS_BIT(ch))); // CH_EN_WE=1, CH_EN=0
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

/*****************************************************************************/
// disable_chs()
/*****************************************************************************/
static FDMA_INLINE void disable_chs(vtss_state_t *const vstate, int ch_mask)
{
    VCOREII_WR(vstate, VTSS_MISC_CH_EN_REG, VTSS_F_CH_EN_WE(ch_mask)); // CH_EN_WE=1, CH_EN=0
}

/*****************************************************************************/
// enable_block_done_intr()
/*****************************************************************************/
static FDMA_INLINE void enable_block_done_intr(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    VTSS_OS_REORDER_BARRIER();
    VCOREII_WR(vstate, VTSS_INTR_MASK_BLOCK, VTSS_F_INT_MASK_WE_BLOCK(VTSS_BIT(ch)) | VTSS_F_INT_MASK_BLOCK(VTSS_BIT(ch)));
}

/*****************************************************************************/
// enable_tfr_done_intr()
/*****************************************************************************/
static FDMA_INLINE void enable_tfr_done_intr(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    VTSS_OS_REORDER_BARRIER();
    VCOREII_WR(vstate, VTSS_INTR_MASK_TFR, VTSS_F_INT_MASK_WE_TFR(VTSS_BIT(ch)) | VTSS_F_INT_MASK_TFR(VTSS_BIT(ch)));
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// disable_block_done_intr()
/*****************************************************************************/
static FDMA_INLINE void disable_block_done_intr(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    VCOREII_WR(vstate, VTSS_INTR_MASK_BLOCK, VTSS_F_INT_MASK_WE_BLOCK(VTSS_BIT(ch)));
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// disable_tfr_done_intr()
/*****************************************************************************/
static FDMA_INLINE void disable_tfr_done_intr(vtss_state_t *const vstate, vtss_fdma_ch_t ch)
{
    VCOREII_WR(vstate, VTSS_INTR_MASK_TFR, VTSS_F_INT_MASK_WE_TFR(VTSS_BIT(ch)));
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

/*****************************************************************************/
// clear_pending_block_done_intr()
/*****************************************************************************/
static FDMA_INLINE void clear_pending_block_done_intr(vtss_state_t *const vstate, int mask)
{
    VCOREII_WR(vstate, VTSS_INTR_CLEAR_BLOCK, VTSS_F_CLEAR_BLOCK(mask));
}

/*****************************************************************************/
// clear_pending_tfr_done_intr()
/*****************************************************************************/
static FDMA_INLINE void clear_pending_tfr_done_intr(vtss_state_t *const vstate, int mask)
{
    VCOREII_WR(vstate, VTSS_INTR_CLEAR_TFR, VTSS_F_CLEAR_TFR(mask));
}

/*****************************************************************************/
// l28_fdma_xtr_dcb_init()
/*****************************************************************************/
static vtss_rc l28_fdma_xtr_dcb_init(vtss_state_t *const vstate, const vtss_fdma_ch_t xtr_ch, fdma_sw_dcb_t *list, u32 cfg_alloc_len)
{
    fdma_hw_dcb_t *hw_dcb = FDMA_HW_DCB(list);

    // Initialize the DCB area
    hw_dcb->sar  = CPU_TO_BUS(FDMA_SAR_CH_ID(xtr_ch) | FDMA_SAR_CHUNK_SIZE(cfg_alloc_len));
    hw_dcb->dar  = CPU_TO_BUS(VTSS_OS_VIRT_TO_PHYS(list->ifh_ptr));
    hw_dcb->llp  = CPU_TO_BUS(FDMA_USER_DCB(list)->next ? VTSS_OS_VIRT_TO_PHYS(FDMA_HW_DCB(FDMA_SW_DCB(FDMA_USER_DCB(list)->next))) : 0);
    hw_dcb->ctl0 = CPU_TO_BUS(VTSS_F_LLP_SRC_EN      | VTSS_F_LLP_DST_EN      | VTSS_F_SMS(1)  | VTSS_F_TT_FC(4) |
                              VTSS_F_SRC_MSIZE(2)    | VTSS_F_DEST_MSIZE(3)   | VTSS_F_SINC(2) |
                              VTSS_F_SRC_TR_WIDTH(2) | VTSS_F_DST_TR_WIDTH(2) | VTSS_F_INT_EN);
    hw_dcb->ctl1 = hw_dcb->sstat = 0;
    hw_dcb->dstat = 0x80000000; // Set a bit that we know will always be cleared once the FDMA has updated this field by reading VTSS_ICPU_CFG_GPDMA_FDMA_XTR_LAST_CHUNK_STAT[xtr_ch].

    VTSS_OS_REORDER_BARRIER();

    // Flush the dcache, so that the DCB gets written to main memory.
    VTSS_OS_DCACHE_FLUSH(hw_dcb, VTSS_FDMA_DCB_SIZE_BYTES);

    return VTSS_RC_OK;
}

/*****************************************************************************/
// l28_fdma_xtr_connect()
// This function must be called with interrupts disabled, so that no context
// switches can occur, since it manipulates list pointers that are used in
// IRQ (DSR) context.
/*****************************************************************************/
static void l28_fdma_xtr_connect(vtss_state_t *const vstate, fdma_hw_dcb_t *tail, fdma_hw_dcb_t *more)
{
    tail->llp = CPU_TO_BUS(VTSS_OS_VIRT_TO_PHYS(more));
    VTSS_OS_REORDER_BARRIER();

    // Flush the dcache, so that the previous tail's DCB's LLP gets written to main memory.
    VTSS_OS_DCACHE_FLUSH(&tail->llp, 4);
}

/*****************************************************************************/
// l28_fdma_xtr_restart_ch()
// This function must be called with interrupts disabled, so that no context
// switches can occur, since it manipulates list pointers that are used in
// IRQ (DSR) context.
/*****************************************************************************/
static BOOL l28_fdma_xtr_restart_ch(vtss_state_t *const vstate, vtss_fdma_ch_t xtr_ch)
{
    fdma_ch_state_t *ch_state = &vstate->fdma_state.fdma_ch_state[xtr_ch];

    if (ch_state->cur_head) {
        // As long as we haven't yet used all DCBs, don't restart.
        return FALSE;
    }

    if ((VCOREII_RD(vstate, VTSS_MISC_CH_EN_REG) & VTSS_F_CH_EN(VTSS_BIT(xtr_ch))) != 0) {
        // Channel not disabled yet.
        return FALSE;
    }

    // Re-enable the channel
    VCOREII_WR(vstate, VTSS_CH_LLP(xtr_ch), VTSS_OS_VIRT_TO_PHYS(ch_state->free_head));
    enable_ch(vstate, xtr_ch);
    return TRUE;
}

/*****************************************************************************/
// inj_ch_init()
/*****************************************************************************/
static void inj_ch_init(vtss_state_t *const vstate, fdma_hw_dcb_t *hw_dcb, vtss_fdma_ch_t ch, u32 phys_port)
{
    fdma_state_t *state       = &vstate->fdma_state;
    fdma_ch_state_t *ch_state = &state->fdma_ch_state[ch];
    u32 port_as_mask = 1UL << phys_port;

    // Use Egress Device or CPU Port Module for injection.
    // Only re-program the DMA controller if the egress port has changed.
    if (ch_state->phys_port != phys_port) {
        // We need to re-configure the DMA channel to inject to another front port.
        if (phys_port < 16) {
            VCOREII_WR(vstate, GPDMA_FDMA_CH_CFG_0(ch), VTSS_F_MODULE(1) | VTSS_F_SUB_MODULE((phys_port -  0)) | VTSS_F_DATA_ADDR(0xC0));
        } else {
            VCOREII_WR(vstate, GPDMA_FDMA_CH_CFG_0(ch), VTSS_F_MODULE(6) | VTSS_F_SUB_MODULE((phys_port - 16)) | VTSS_F_DATA_ADDR(0xC0));
        }

        if (phys_port == VTSS_CPU_PM_NUMBER) {
            VCOREII_WR(vstate, GPDMA_FDMA_INJ_CFG(ch), VTSS_F_INJ_HDR_ENA);
#if defined(VTSS_FEATURE_VSTAX)
        } else if ((port_as_mask & vstate->vstax_info.chip_info[0].mask_a) || (port_as_mask & vstate->vstax_info.chip_info[0].mask_b)) {
            VCOREII_WR(vstate, GPDMA_FDMA_INJ_CFG(ch), VTSS_F_INJ_CFG_WR_DELAY(VTSS_FDMA_INJ_WR_DELAY_2_5G_PORTS));
#endif /* defined(VTSS_FEATURE_VSTAX) */
        } else {
            VCOREII_WR(vstate, GPDMA_FDMA_INJ_CFG(ch), VTSS_F_INJ_CFG_WR_DELAY(VTSS_FDMA_INJ_WR_DELAY_1G_PORTS));
        }

        // Set-up automated load of DCBs
        VCOREII_WR(vstate, GPDMA_FDMA_CH_CFG_1(ch), VTSS_F_USAGE | VTSS_F_FC_INJ_ENA); // USAGE=injection and FC_INJ_ENA
        VCOREII_WR(vstate, VTSS_CH_CTL0(ch), VTSS_F_LLP_SRC_EN | VTSS_F_LLP_DST_EN); // We just need to enable LLP_SRC_EN and LLP_DST_EN here since the first descriptor overwrites the remaining fields anyway.

        if (phys_port < VTSS_CPU_PM_NUMBER) {
            VCOREII_WR(vstate, VTSS_CH_CFG0(ch), VTSS_F_LOCK_CH | VTSS_F_HS_SEL_SRC | VTSS_F_HS_SEL_DST | VTSS_F_CH_PRIOR(ch_state->prio)); // HS_SEL_SRC=HS_SEL_DST=1=Software Initiated. Use channel locking, so that no other channels are granted for the whole transfer (in order to avoid interleaving data)
            VCOREII_WR(vstate, VTSS_CH_CFG1(ch), VTSS_F_PROTCTL(1)); // PROTCTL=1, DST_PER=inj_ch, FCMODE=0; // Well, maybe not when injecting into egress port?
        } else {
            VCOREII_WR(vstate, VTSS_CH_CFG0(ch), VTSS_F_LOCK_CH | VTSS_F_HS_SEL_SRC | VTSS_F_CH_PRIOR(ch_state->prio));  // HS_SEL_SRC=1=Software Initiated, HS_SEL_DST=0=H/W Initiated. Use channel locking, so that no other channels are granted for the whole transfer (in order to avoid interleaving data)
            VCOREII_WR(vstate, VTSS_CH_CFG1(ch), VTSS_F_DST_PER(ch) | VTSS_F_PROTCTL(1) | VTSS_F_FCMODE); // PROTCTL=1, DST_PER=inj_ch, FCMODE=1; // Well, maybe not when injecting into egress port?
        }
        ch_state->phys_port = phys_port;
    }

    VCOREII_WR(vstate, VTSS_CH_LLP(ch), VTSS_OS_VIRT_TO_PHYS(hw_dcb));
}

/*****************************************************************************/
// l28_fdma_inj_restart_ch()
/*****************************************************************************/
static vtss_rc l28_fdma_inj_restart_ch(vtss_state_t *const vstate, vtss_fdma_ch_t inj_ch, fdma_sw_dcb_t *head, BOOL do_start, vtss_trace_group_t trc_grp)
{
    fdma_port_state_t *port_state = &vstate->fdma_state.fdma_port_state[head->phys_port];

    if (do_start) {
        FDMA_CHECK_RC(trc_grp, (VCOREII_RD(vstate, VTSS_MISC_CH_EN_REG) & VTSS_BIT(inj_ch)) == 0);

        // Re-initialize the state retransmit count for the new frame.
        port_state->state_retransmit_cnt = port_state->cfg_retransmit_cnt;

        // Start the channel if allowed to.
        if(!port_state->suspended) {
            port_state->active_cnt++;
            inj_ch_init(vstate, FDMA_HW_DCB(head), inj_ch, head->phys_port);
            enable_ch(vstate, inj_ch);
        }
    } else {
        // Since the tail is non-zero, we know that the interrupt handler will get invoked
        // at some point in time, so if VTSS_MISC_CH_EN_REG(inj_ch)==1, the DMA controller is
        // not yet done with the previous frame. If it's 0, the previous frame is done and
        // we must be having a pending interrupt. Well, if the destination port is suspended
        // it may be that upper layers have added a packet, so that tail is non-zero when
        // they try to add a second packet. So suspended needs to be part of the assertion
        // below.
        FDMA_CHECK_RC(trc_grp, port_state->suspended                                 ||
                      ((VCOREII_RD(vstate, VTSS_MISC_CH_EN_REG)  & VTSS_BIT(inj_ch)) ||
                       (VCOREII_RD(vstate, VTSS_INTR_STATUS_TFR) & VTSS_BIT(inj_ch))));
    }

    return VTSS_RC_OK;
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// inj_insert_vlan_tag()
// #ptr points four bytes ahead of current DMAC.
// #tpid is ethertype.
// #tci is the Tag Control Identifier, i.e. the combined PCP/CFI/VID.
/*****************************************************************************/
static void inj_insert_vlan_tag(u8 *ptr, vtss_etype_t tpid, u16 tci)
{
    int i;

    // DMAC & SMAC four bytes ahead (memcpy potentially fails due to overlapping regions):
    for (i = 0; i < SIZE_OF_TWO_MAC_ADDRS; i++) {
        *ptr = *(ptr + 4);
        ptr++;
    }

    *(ptr++) = tpid >> 8;
    *(ptr++) = tpid;
    *(ptr++) = tci >> 8;
    *(ptr++) = tci;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/****************************************************************************/
// inj_aggr_code_compute()
// Computes aggregation code, which in turn is stored in the IFH.
// Overcomes a lack of features when it comes to filtered frames sent by the
// CPU. See GNATS #5528.
/****************************************************************************/
static u8 inj_aggr_code_compute(vtss_state_t *vstate, u8 *frm)
{
#define ETYPE_POS         12
#define PROT_HDR_POS      (ETYPE_POS + 2)
#define VERS_POS          (PROT_HDR_POS +  0)
#define HLEN_POS          (PROT_HDR_POS +  0)
#define FLAGS_POS         (PROT_HDR_POS +  6)
#define FRAG_OFFS_MSB_POS (PROT_HDR_POS +  6)
#define FRAG_OFFS_LSB_POS (PROT_HDR_POS +  7)
#define PROTO_POS         (PROT_HDR_POS +  9)
#define SIP_LSB_POS       (PROT_HDR_POS + 15)
#define DIP_LSB_POS       (PROT_HDR_POS + 19)
#define SRC_PORT_LSB_POS  (PROT_HDR_POS + 21)
#define DST_PORT_LSB_POS  (PROT_HDR_POS + 23)
  vtss_aggr_mode_t *mode = &vstate->aggr_mode;

  u8 aggr_code = 0;
  // We only operate on IPv4 (computation of aggregation code on IPv6 frames is currently not supported by this piece of S/W):
  if ((frm[ETYPE_POS] != 0x08) || (frm[ETYPE_POS + 1] != 0x00) || (((frm[VERS_POS] >> 4) & 0xF) != 4)) {
    return aggr_code;
  }

  if (mode->sip_dip_enable) {
    aggr_code ^= frm[SIP_LSB_POS] ^ (frm[SIP_LSB_POS] >> 4);
    aggr_code ^= frm[DIP_LSB_POS] ^ (frm[DIP_LSB_POS] >> 4);
  }

  if (mode->sport_dport_enable) {
    // If HLEN == 5 32-bit words     &&  more fragments == 0                   &&
    if (((frm[HLEN_POS] & 0xF) == 5) && (((frm[FLAGS_POS] >> 5) & 0x1) == 0)   &&
       // fragment offset == 0                                                 &&
       ((frm[FRAG_OFFS_MSB_POS] & 0x1F) == 0) && (frm[FRAG_OFFS_LSB_POS] == 0) &&
       // (Protocol     == TCP   ||  Protocol       == UDP)
       ((frm[PROTO_POS] == 0x06) || (frm[PROTO_POS] == 0x11))) {
      aggr_code ^= frm[SRC_PORT_LSB_POS] ^ (frm[SRC_PORT_LSB_POS] >> 4);
      aggr_code ^= frm[DST_PORT_LSB_POS] ^ (frm[DST_PORT_LSB_POS] >> 4);
    }
  }

  // mode->smac_enable and mode->dmac_enable is not part of the categorizer aggregation code computation (it's in the analyzer).

  return aggr_code & 0xF; // Only 4 bits.

#undef ETYPE_POS
#undef PROT_HDR_POS
#undef VERS_POS
#undef HLEN_POS
#undef FLAGS_POS
#undef FRAG_OFFS_MSB_POS
#undef FRAG_OFFS_LSB_POS
#undef PROTO_POS
#undef SIP_LSB_POS
#undef DIP_LSB_POS
#undef SRC_PORT_LSB_POS
#undef DST_PORT_LSB_POS
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#define FDMA_CMD_PUT(_cmd_, _val_, _field_) (_cmd_) |= VTSS_ENCODE_BITFIELD  (_val_, FDMA_CMD_F_      ## _field_ ## _FPOS, FDMA_CMD_F_      ## _field_ ## _FLEN)

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// inj_ifh_init()
//
// If VTSS_OPT_FDMA_VER == 1, @data_ptr is expected to point to the beginning
// of the IFH (8 bytes), after which the CMD field comes (4 bytes), after
// which the payload including a possible VStaX header is expected to come.
//
// If VTSS_OPT_FDMA_VER == 2, @data_ptr is expected to point to
// VTSS_FDMA_HDR_SIZE_BYTES of space after which the payload comes. The
// payload data must not contain a VStaX header, so it comes as sideband
// information in the @props, and is inserted by this function into
// the correct place of the payload if @props->contains_stack_hdr is TRUE.
//
// In case of an error, the function returns -1. Otherwise it returns the
// offset into (u8 *)@data_ptr where the the start of the IFH is located.
// If VTSS_OPT_FDMA_VER == 1, this offset will always be 0.
//
// Returns 0xFFFFFFFF on error, an offset to start of IFH on success.
/*****************************************************************************/
static FDMA_INLINE u32 inj_ifh_init(vtss_state_t *const vstate, u8 *const data_ptr, const u32 port, u32 frm_sz_bytes_excl_ifh_cmd_incl_fcs, const vtss_fdma_inj_props_t *const props)
{
    u64           inj_hdr   = 0;
    u32           cmd       = 0;
    fdma_state_t *state     = &vstate->fdma_state;
    u8            qos_class = props->qos_class;
    int           ifh_offset;

    if (qos_class == 8) {
      qos_class = 7;
    }

#if VTSS_OPT_FDMA_VER == 2
    // Variable offset to first byte of IFH depending on whether the VStaX header
    // is going to be inserted or not.
    ifh_offset = VTSS_FDMA_HDR_SIZE_BYTES - VTSS_FDMA_INJ_HDR_SIZE_BYTES;
#if defined(VTSS_FEATURE_VSTAX)
    if (props->contains_stack_hdr) {
        // Gotta make room for the VStaX header in the payload data.
        ifh_offset -= VTSS_VSTAX_HDR_SIZE;

        // Copy the DMAC and SMAC VTSS_VSTAX_HDR_SIZE bytes ahead.
        memcpy(data_ptr + VTSS_FDMA_HDR_SIZE_BYTES - VTSS_VSTAX_HDR_SIZE, data_ptr + VTSS_FDMA_HDR_SIZE_BYTES, VTSS_VSTAX_HDR_SIZE);

        // Insert the VStaX header into the payload
        memcpy(data_ptr + VTSS_FDMA_HDR_SIZE_BYTES - VTSS_VSTAX_HDR_SIZE + SIZE_OF_TWO_MAC_ADDRS, props->stack_hdr_bin, VTSS_VSTAX_HDR_SIZE);

        // The frame then becomes VTSS_VSTAX_HDR_SIZE bigger
        frm_sz_bytes_excl_ifh_cmd_incl_fcs += VTSS_VSTAX_HDR_SIZE;
    } else
#endif /* VTSS_FEATURE_VSTAX */
    if (props->switch_frm == FALSE && props->tpid != 0) {
        ifh_offset -= 4;
        inj_insert_vlan_tag(data_ptr + VTSS_FDMA_HDR_SIZE_BYTES - 4, props->tpid,  (qos_class << 13) | (props->vlan & 0xFFF));
        frm_sz_bytes_excl_ifh_cmd_incl_fcs += 4;
    }
#else
    ifh_offset = 0;
#endif /* VTSS_OPT_FDMA_VER == 2 */

    if (frm_sz_bytes_excl_ifh_cmd_incl_fcs < 64) {
        frm_sz_bytes_excl_ifh_cmd_incl_fcs = 64;
    }

    #define FDMA_IFH_PUT(_ifh_, _val_, _field_) (_ifh_) |= VTSS_ENCODE_BITFIELD64(_val_, VTSS_FDMA_IFH_F_ ## _field_ ## _FPOS, VTSS_FDMA_IFH_F_ ## _field_ ## _FLEN)

    // At least when using the CPU PM to inject the frame, the source_port field of the IFH must be set or the frame will be aged, because we don't set the timestamp in the IFH.
    FDMA_IFH_PUT(inj_hdr, frm_sz_bytes_excl_ifh_cmd_incl_fcs, LEN);
    FDMA_IFH_PUT(inj_hdr, port,                               SRC_PORT);
    FDMA_IFH_PUT(inj_hdr, props->vlan,                        TAG);
    // qos_class is specified as a number in range [0; 7], but Lu28 only accepts [0; 3], so here we simply right shift by one to let the three MSbits rule.
    FDMA_IFH_PUT(inj_hdr, qos_class >> 1,                     QOS);

#if defined(VTSS_FEATURE_VSTAX)
    if (props->contains_stack_hdr) {
        u32 port_as_mask = 1UL << (u32)port;
        FDMA_IFH_PUT(inj_hdr, 1, STACK_HDR);
        if (port_as_mask & vstate->vstax_info.chip_info[0].mask_a) {
            FDMA_IFH_PUT(inj_hdr, 1, STACK_LINK_A);
        } else if (port_as_mask & vstate->vstax_info.chip_info[0].mask_b) {
            FDMA_IFH_PUT(inj_hdr, 1, STACK_LINK_B);
        } else {
            // Cannot inject stack-headered frame on non-stack-port as we don't
            // know what to set the stack_info.stack_link bits to.
            // Allow these options to be used if this function is not writing the real IFH, though!
            FDMA_ASSERT(NORMAL, props->dont_touch_ifh, return 0xFFFFFFFF;);
        }
    }
#endif /* VTSS_FEATURE_VSTAX */

    // Initialize the CMD
    if (port == VTSS_CPU_PM_NUMBER) {
        // We have the option of switching the frame. If so, we set the 'do_analyze' flag and leave the port mask 0.
        // Otherwise, we leave the 'do_analyze' 0 and set the 'frame_dest' to whatever the user wants.
        // In both cases we set the CMD field's 'tx' flag.
        if (props->switch_frm) {
            FDMA_IFH_PUT(inj_hdr, props->aggr_code, AGGR_CODE);
            FDMA_CMD_PUT(cmd, 1, DOANALYZE);
        } else {
            FDMA_CMD_PUT(cmd, props->port_mask, FRAMEDEST);
        }

        FDMA_CMD_PUT(cmd, 1, CPU_TX);
    } else {
        // Set the TX flag if we're not in Tx flow control mode.
        // In flow control mode we check for overflow after
        // FDMA controlled injection and set the flag in S/W if
        // no overflow occurred.
        if (state->fdma_port_state[port].cfg_tx_flow_ctrl == FALSE) {
            FDMA_CMD_PUT(cmd, 1, CPU_TX);
        }
    }

    if (!props->dont_touch_ifh) {
        data_ptr[ifh_offset + 0] = inj_hdr >> 32;
        data_ptr[ifh_offset + 1] = inj_hdr >> 40;
        data_ptr[ifh_offset + 2] = inj_hdr >> 48;
        data_ptr[ifh_offset + 3] = inj_hdr >> 56;
        data_ptr[ifh_offset + 4] = inj_hdr >>  0;
        data_ptr[ifh_offset + 5] = inj_hdr >>  8;
        data_ptr[ifh_offset + 6] = inj_hdr >> 16;
        data_ptr[ifh_offset + 7] = inj_hdr >> 24;
    }

    data_ptr[ifh_offset +  8] = cmd >>  0;
    data_ptr[ifh_offset +  9] = cmd >>  8;
    data_ptr[ifh_offset + 10] = cmd >> 16;
    data_ptr[ifh_offset + 11] = cmd >> 24;
    return ifh_offset;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/*****************************************************************************/
// inj_ifh_init()
/*****************************************************************************/
static vtss_rc inj_ifh_init(vtss_state_t *const vstate, fdma_sw_dcb_t *sw_dcb, vtss_packet_tx_info_t *const tx_info, vtss_fdma_ch_t *inj_ch, u32 *frm_size)
{
    vtss_fdma_list_t *user_dcb = FDMA_USER_DCB(sw_dcb), *user_dcb_iter;
    u32              bytes_stripped_on_egress = 0, actual_length_bytes = 0, bin_hdr_len = 8 /* VTSS_FDMA_IFH_SIZE_BYTES */, cmd = 0;
    vtss_chip_no_t   chip_no;
    vtss_port_no_t   port_no = 0, stack_port_no = VTSS_PORT_NO_NONE;
    u32              port_cnt = 0;
    u64              chip_port_mask = 0;

    // Frame pointer must be equal to the IFH pointer by now.
    FDMA_ASSERT(NORMAL, sw_dcb->ifh_ptr == user_dcb->frm_ptr, return VTSS_RC_ERROR;);

    // The FDMA must either use the CPU Port Module or a direct port. Which, depends on
    // the dst_port_mask and whether it is being sent switched.
    // VStaX frame must also be sent non-switched.
    if (tx_info->switch_frm == FALSE) {
        VTSS_RC(vtss_cmn_logical_to_chip_port_mask(vstate, tx_info->dst_port_mask, &chip_port_mask, &chip_no, &stack_port_no, &port_cnt, &port_no));
    }

#if defined(VTSS_FEATURE_VSTAX)
    if (tx_info->tx_vstax_hdr != VTSS_PACKET_TX_VSTAX_NONE) {

        FDMA_ASSERT(NORMAL, port_cnt == 1, return VTSS_RC_ERROR;);

        // Insert VTSS_VSTAX_HDR_SIZE VStaX header in place of the current Ethertype.
        // Start by moving the IFH pointer.
        sw_dcb->ifh_ptr   -= VTSS_VSTAX_HDR_SIZE;
        user_dcb->act_len += VTSS_VSTAX_HDR_SIZE;

        // Move the DMAC and SMAC VTSS_VSTAX_HDR_SIZE bytes ahead.
        memcpy(sw_dcb->ifh_ptr, user_dcb->frm_ptr, VTSS_VSTAX_HDR_SIZE);

        if (tx_info->tx_vstax_hdr == VTSS_PACKET_TX_VSTAX_SYM) {
             // The application hasn't yet encoded the VStaX header. Do it for it.
             // Get the stack port from the dst_port_mask.
             FDMA_ASSERT(NORMAL, stack_port_no != VTSS_PORT_NO_NONE, return VTSS_RC_ERROR;);
             VTSS_RC(vstate->cil_func.vstax_header2frame(vstate, stack_port_no, &tx_info->vstax.sym, user_dcb->frm_ptr));
        } else {
            // Insert the already encoded VStaX header into the payload
            memcpy(user_dcb->frm_ptr, tx_info->vstax.bin, VTSS_VSTAX_HDR_SIZE);
        }

        // Now adjust the frame pointer as well.
        user_dcb->frm_ptr -= VTSS_VSTAX_HDR_SIZE;
    } else
#endif /* defined(VTSS_FEATURE_VSTAX) */

    if (tx_info->switch_frm == FALSE && tx_info->tag.tpid != 0) {
        // When not switching, we insert a VLAN tag into the frame when tag.tpid != 0.
        VTSS_RC(vtss_fdma_cmn_insert_vlan_tag(sw_dcb, &tx_info->tag, FALSE /* No need to move IFH data because we haven't yet constructed it */, FALSE /* Called with interrupts enabled */));
    }

    if (tx_info->switch_frm) {
        tx_info->aggr_code = inj_aggr_code_compute(vstate, user_dcb->frm_ptr);
    }

    // We got to adjust the length of the EOF DCB to accommodate for a
    // minimum sized Ethernet frame, and we also need the full frame size
    // for the injection header encode function.
    user_dcb_iter = user_dcb;
    while (user_dcb_iter) {
        actual_length_bytes += user_dcb_iter->act_len;

        if (user_dcb_iter->next == NULL) {
            // End-of-frame DCB. All lengths are excluding IFH/CMD and
            // excluding FCS, hence the 60 instead of 64.
            // We cannot pad the actual frame data with zeros because we
            // don't know how much data was allocated by the application.
            u32 bytes_on_wire = actual_length_bytes - bytes_stripped_on_egress;
            if (bytes_on_wire < 60) {
                u32 pad_bytes = 60 - bytes_on_wire;
                user_dcb_iter->act_len += pad_bytes;
                actual_length_bytes    += pad_bytes;
            }
        }

        user_dcb_iter = user_dcb_iter->next;
    }

    tx_info->frm_len = actual_length_bytes;     /* Exclude IFH, CMD, and FCS            */
    *frm_size        = actual_length_bytes + 4; /* Exclude IFH and CMD, but include FCS */

    // The rest of this driver requires the physical port number held in the SOF DCB.
    // We use the CPU Port Module is sending switched or to multiple ports, otherwise
    // we direct it to the port in question.
    sw_dcb->phys_port = tx_info->switch_frm || port_cnt > 1 ? VTSS_CPU_PM_NUMBER : vstate->port_map[port_no].chip_port;

    // Now insert the CMD field.
    if (sw_dcb->phys_port == VTSS_CPU_PM_NUMBER) {
        // We have the option of switching the frame. If so, we set the 'do_analyze' flag and leave the port mask 0.
        // Otherwise, we leave the 'do_analyze' 0 and set the 'frame_dest' to whatever the user wants.
        // In both cases we set the CMD field's 'tx' flag.
        if (tx_info->switch_frm) {
            FDMA_CMD_PUT(cmd, 1, DOANALYZE);
        } else {
            FDMA_CMD_PUT(cmd, chip_port_mask, FRAMEDEST);
        }

        FDMA_CMD_PUT(cmd, 1, CPU_TX);
    } else {
        // Set the TX flag if we're not in Tx flow control mode.
        // In flow control mode we check for overflow after
        // FDMA controlled injection and set the flag in S/W if
        // no overflow occurred.
        if (vstate->fdma_state.fdma_port_state[sw_dcb->phys_port].cfg_tx_flow_ctrl == FALSE) {
            FDMA_CMD_PUT(cmd, 1, CPU_TX);
        }
    }

    sw_dcb->ifh_ptr -= 4; /* VTSS_FDMA_CMD_SIZE_BYTES */
    sw_dcb->ifh_ptr[0] = cmd >>  0;
    sw_dcb->ifh_ptr[1] = cmd >>  8;
    sw_dcb->ifh_ptr[2] = cmd >> 16;
    sw_dcb->ifh_ptr[3] = cmd >> 24;

    if (sw_dcb->phys_port == VTSS_CPU_PM_NUMBER) {
        *inj_ch = FDMA_CH_TX_SWITCHED; // Both multicast and switched frames.
#if defined(VTSS_FEATURE_VSTAX)
    } else if (stack_port_no != VTSS_PORT_NO_NONE) {
        if (stack_port_no == vstate->vstax_conf.port_0) {
            *inj_ch = FDMA_CH_TX_STACK_A;
        } else {
            *inj_ch = FDMA_CH_TX_STACK_B;
        }
#endif /* defined(VTSS_FEATURE_VSTAX) */
    } else {
        *inj_ch = FDMA_CH_TX_PORTSPEC;
    }

    // And finally time for the IFH
    sw_dcb->ifh_ptr -= bin_hdr_len;
    VTSS_RC(vstate->cil_func.tx_hdr_encode(vstate, tx_info, sw_dcb->ifh_ptr, &bin_hdr_len));

    // Check that we haven't inserted too much data.
    FDMA_ASSERT(NORMAL, sw_dcb->ifh_ptr >= sw_dcb->data, return VTSS_RC_ERROR;);

    VTSS_IG(VTSS_TRACE_GROUP_FDMA_NORMAL, "IFH (%u bytes) + a bit frame data (%u bytes incl. FCS):", FDMA_USER_DCB(sw_dcb)->frm_ptr - sw_dcb->ifh_ptr, *frm_size);
    VTSS_IG_HEX(VTSS_TRACE_GROUP_FDMA_NORMAL, sw_dcb->ifh_ptr, 64);

    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// inj_dcb_init()
// @sw_dcb : NULL-terminated List of DCBs
// @inj_ch: The channel that this one goes through.
// In debug mode, it also takes a pointer to an integer, whl_frm_sz_bytes,
// which is filled with the total frame size, including IFH, CMD, and FCS,
// computed based on each fragment size.
// The function returns a pointer to the last list item used for this frame.
/*****************************************************************************/
static FDMA_INLINE fdma_sw_dcb_t *inj_dcb_init(const vtss_fdma_ch_t inj_ch, fdma_sw_dcb_t *sw_dcb, u32 ifh_offset, int *const whl_frm_sz_bytes, u32 *const dcb_cnt)
{
    u32              *phys_data_ptr, phys_data_off;
    fdma_hw_dcb_t    *hw_dcb;
    u8               *data_ptr;
    int              frag_sz_bytes;
    int              sof = 1, eof, port = sw_dcb->phys_port;
    fdma_sw_dcb_t    *tail = sw_dcb;
    vtss_fdma_list_t *user_dcb;

    *whl_frm_sz_bytes = 0;
    *dcb_cnt          = 0;

    while (sw_dcb) {
        user_dcb      = FDMA_USER_DCB(sw_dcb);
        frag_sz_bytes = user_dcb->act_len;

        FDMA_ASSERT(NORMAL, sw_dcb->data, return NULL;);
        FDMA_ASSERT(NORMAL,
          ((frag_sz_bytes >= VTSS_FDMA_MIN_DATA_PER_INJ_SOF_DCB_BYTES && sof == 1)  ||
           (frag_sz_bytes >= VTSS_FDMA_MIN_DATA_PER_NON_SOF_DCB_BYTES && sof == 0)) &&
            frag_sz_bytes <= VTSS_FDMA_MAX_DATA_PER_DCB_BYTES, return NULL;);

        if (sof) {
            // The position of the IFH into sw_dcb->data may vary (if VTSS_OPT_FDMA_VER >= 2),
            // so we need to add @ifh_offset to the sw_dcb->data before storing it in the DCB
            // and subtract @ifh_offset from the frag_sz_bytes before writing the length.
            frag_sz_bytes -= ifh_offset;
            data_ptr = sw_dcb->data + ifh_offset;
            sw_dcb->ifh_ptr = data_ptr;
            user_dcb->frm_ptr = sw_dcb->ifh_ptr + VTSS_FDMA_INJ_HDR_SIZE_BYTES;
        } else {
            // Non-SOF DCBs aren't changed.
            data_ptr = sw_dcb->data;
            sw_dcb->ifh_ptr = user_dcb->frm_ptr = sw_dcb->data;
        }

        *whl_frm_sz_bytes += frag_sz_bytes;

        eof = user_dcb->next == NULL;
        if (eof) {
            // whl_frm_sz_bytes includes INJ header the frame itself and the FCS.
            // We gotta adjust the length of the last fragment to accommodate for a minimum-sized Ethernet frame.
            if (*whl_frm_sz_bytes < 64 + VTSS_FDMA_INJ_HDR_SIZE_BYTES) {
                frag_sz_bytes += (64 + VTSS_FDMA_INJ_HDR_SIZE_BYTES - *whl_frm_sz_bytes);
            }
        }

        // Due to a bug in the FDMA, all non-EOF DCBs' associated data area cannot have a size that is not a multiple of 4.
        // This is documented in GNATS #5355.
        // For testing, it is possible to turn off this check.
#if !defined(VTSS_FDMA_IGNORE_GNATS_5355)
        FDMA_ASSERT(NORMAL, eof || (frag_sz_bytes & 0x3) == 0, return NULL;);
#endif /* !defined(VTSS_FDMA_IGNORE_GNATS_5355) */

        phys_data_ptr = (u32 *)VTSS_OS_VIRT_TO_PHYS(data_ptr);
        phys_data_off = (u32)phys_data_ptr & 0x3;
        hw_dcb = FDMA_HW_DCB(sw_dcb);

        // The H/W DCB should be cache-line-size aligned
        FDMA_ASSERT(NORMAL, ((u32)hw_dcb & (VTSS_OS_DCACHE_LINE_SIZE_BYTES - 1)) == 0, return NULL;);

        hw_dcb->sar  = CPU_TO_BUS((u32)phys_data_ptr & ~0x3);
        hw_dcb->dar  = CPU_TO_BUS((FDMA_DAR_EOF(eof) | FDMA_DAR_SOF(sof) | FDMA_DAR_CH_ID(inj_ch) | FDMA_DAR_CHUNK_SIZE(frag_sz_bytes) | FDMA_DAR_SAR_OFFSET(phys_data_off)));
        hw_dcb->llp  = CPU_TO_BUS(user_dcb->next ? VTSS_OS_VIRT_TO_PHYS(FDMA_HW_DCB(FDMA_SW_DCB(user_dcb->next))) : 0); // NULL
        hw_dcb->ctl0 = CPU_TO_BUS(VTSS_F_LLP_SRC_EN | VTSS_F_LLP_DST_EN | VTSS_F_DMS(1) | VTSS_F_SRC_MSIZE(3) | VTSS_F_DEST_MSIZE(3) | VTSS_F_DINC(2) | VTSS_F_SRC_TR_WIDTH(2) | VTSS_F_DST_TR_WIDTH(2) | eof); // We only want interrupts if this is the EOF.
        if (port == VTSS_CPU_PM_NUMBER) {
            hw_dcb->ctl0 |= CPU_TO_BUS(VTSS_F_TT_FC(1));
        }

        // If the data area is non-32-bit-aligned (as is the case for Linux), then we need to check if we're gonna inject one more 32-bit word.
        hw_dcb->ctl1  = CPU_TO_BUS((frag_sz_bytes + phys_data_off + 3) / 4); // BLOCK_TS
        hw_dcb->sstat = hw_dcb->dstat = CPU_TO_BUS(0);

        VTSS_OS_REORDER_BARRIER();

        // Flush the dcache, so that the DCB gets written to main memory.
        VTSS_OS_DCACHE_FLUSH(hw_dcb, VTSS_FDMA_DCB_SIZE_BYTES);

        VTSS_OS_REORDER_BARRIER();

        // Flush the dcache, so that the frame data gets written to main memory.
        VTSS_OS_DCACHE_FLUSH(data_ptr, frag_sz_bytes);

        (*dcb_cnt)++;

        // Prepare for next iteration
        sof = 0;
        if (user_dcb->next) {
            tail = FDMA_SW_DCB(user_dcb->next);
        }

        sw_dcb = FDMA_SW_DCB(user_dcb->next);
    }
    return tail;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/*****************************************************************************/
// inj_dcb_init()
/*****************************************************************************/
static FDMA_INLINE vtss_rc inj_dcb_init(vtss_state_t *const vstate, fdma_sw_dcb_t *sw_dcb, vtss_fdma_ch_t inj_ch, fdma_sw_dcb_t **tail, u32 *dcb_cnt)
{
    u32              *phys_data_ptr, phys_data_off;
    fdma_hw_dcb_t    *hw_dcb;
    int              frag_sz_bytes;
    int              sof = 1, eof, port = sw_dcb->phys_port;
    vtss_fdma_list_t *user_dcb;

    *dcb_cnt = 0;
    *tail    = sw_dcb;

    while (sw_dcb) {
        user_dcb      = FDMA_USER_DCB(sw_dcb);
        eof           = user_dcb->next == NULL;
        frag_sz_bytes = user_dcb->act_len + (user_dcb->frm_ptr - sw_dcb->ifh_ptr) + (eof ? 4 /* FCS */ : 0);

        FDMA_ASSERT(NORMAL, sw_dcb->ifh_ptr, return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL,
          ((frag_sz_bytes >= VTSS_FDMA_MIN_DATA_PER_INJ_SOF_DCB_BYTES && sof == 1)  ||
           (frag_sz_bytes >= VTSS_FDMA_MIN_DATA_PER_NON_SOF_DCB_BYTES && sof == 0)) &&
            frag_sz_bytes <= VTSS_FDMA_MAX_DATA_PER_DCB_BYTES, return VTSS_RC_ERROR;);

        // Due to a bug in the FDMA, all non-EOF DCBs' associated data area cannot have a size that is not a multiple of 4.
        // This is documented in GNATS #5355.
        // For testing, it is possible to turn off this check.
#if !defined(VTSS_FDMA_IGNORE_GNATS_5355)
        FDMA_ASSERT(NORMAL, eof || (frag_sz_bytes & 0x3) == 0, return VTSS_RC_ERROR;);
#endif /* !defined(VTSS_FDMA_IGNORE_GNATS_5355) */

        phys_data_ptr = (u32 *)VTSS_OS_VIRT_TO_PHYS(sw_dcb->ifh_ptr);
        phys_data_off = (u32)phys_data_ptr & 0x3;
        hw_dcb = FDMA_HW_DCB(sw_dcb);

        // The H/W DCB should be cache-line-size aligned
        FDMA_ASSERT(NORMAL, ((u32)hw_dcb & (VTSS_OS_DCACHE_LINE_SIZE_BYTES - 1)) == 0, return VTSS_RC_ERROR;);

        hw_dcb->sar  = CPU_TO_BUS((u32)phys_data_ptr & ~0x3);
        hw_dcb->dar  = CPU_TO_BUS((FDMA_DAR_EOF(eof) | FDMA_DAR_SOF(sof) | FDMA_DAR_CH_ID(inj_ch) | FDMA_DAR_CHUNK_SIZE(frag_sz_bytes) | FDMA_DAR_SAR_OFFSET(phys_data_off)));
        hw_dcb->llp  = CPU_TO_BUS(user_dcb->next ? VTSS_OS_VIRT_TO_PHYS(FDMA_HW_DCB(FDMA_SW_DCB(user_dcb->next))) : 0); // NULL
        hw_dcb->ctl0 = CPU_TO_BUS(VTSS_F_LLP_SRC_EN | VTSS_F_LLP_DST_EN | VTSS_F_DMS(1) | VTSS_F_SRC_MSIZE(3) | VTSS_F_DEST_MSIZE(3) | VTSS_F_DINC(2) | VTSS_F_SRC_TR_WIDTH(2) | VTSS_F_DST_TR_WIDTH(2) | eof); // We only want interrupts if this is the EOF.
        if (port == VTSS_CPU_PM_NUMBER) {
            hw_dcb->ctl0 |= CPU_TO_BUS(VTSS_F_TT_FC(1));
        }

        // If the data area is non-32-bit-aligned (as is the case for Linux), then we need to check if we're gonna inject one more 32-bit word.
        hw_dcb->ctl1  = CPU_TO_BUS((frag_sz_bytes + phys_data_off + 3) / 4); // BLOCK_TS
        hw_dcb->sstat = hw_dcb->dstat = CPU_TO_BUS(0);

        VTSS_OS_REORDER_BARRIER();

        // Flush the dcache, so that the DCB gets written to main memory.
        VTSS_OS_DCACHE_FLUSH(hw_dcb, VTSS_FDMA_DCB_SIZE_BYTES);

        VTSS_OS_REORDER_BARRIER();

        // Flush the dcache, so that the frame data gets written to main memory.
        VTSS_OS_DCACHE_FLUSH(sw_dcb->ifh_ptr, frag_sz_bytes);

        (*dcb_cnt)++;

        // Prepare for next iteration
        sof = 0;
        if (user_dcb->next) {
            *tail = FDMA_SW_DCB(user_dcb->next);
        }

        sw_dcb = FDMA_SW_DCB(user_dcb->next);
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2
/*****************************************************************************/
// xtr_stack_hdr_move()
/*****************************************************************************/
static FDMA_INLINE void xtr_stack_hdr_move(fdma_sw_dcb_t *sw_dcb)
{
    u8               *dst, *src;
    int              i;
    vtss_fdma_list_t *user_dcb = FDMA_USER_DCB(sw_dcb);

    // Ensure that there's room for the VStaX header just before the IFH.
    FDMA_ASSERT(IRQ, sw_dcb->ifh_ptr - sw_dcb->data >= VTSS_VSTAX_HDR_SIZE, return;);

    // First check if this is a VStaX frame in the first place.
    u16 etype = (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS] << 8) | (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS + 1]);
    if (etype == VTSS_ETYPE_VTSS && (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS + 2] & 0x80)) {

        // It's present. Move it to ->data
        memcpy(sw_dcb->data, user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS, VTSS_VSTAX_HDR_SIZE);

        // Collapse payload by copying MAC addresses to SIZE_OF_TWO_MAC_ADDRS before rest of payload.
        // For the sake of the application, also move the IFH so that the frame comes right after the
        // end of the IFH. This allows the application to find the correct length of the IFH by
        // subtracting sw_dcb->ifh_ptr from sw_dcb->frm_ptr.
        // We need to do it by iteration rather than memcpy() due to possible overlapping addresses
        dst = user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS + VTSS_VSTAX_HDR_SIZE - 1;
        src = user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS                       - 1;
        for (i = VTSS_FDMA_XTR_HDR_SIZE_BYTES + SIZE_OF_TWO_MAC_ADDRS - 1; i >= 0; i--) {
            *(dst--) = *(src--);
        }

        // Adjust length, ifh_ptr, and frm_ptr
        sw_dcb->ifh_ptr   += VTSS_VSTAX_HDR_SIZE;
        user_dcb->frm_ptr += VTSS_VSTAX_HDR_SIZE;
        user_dcb->act_len -= VTSS_VSTAX_HDR_SIZE;
    } else {
        // It's not present. Clear start of IFH to prevent a false hit when
        // application code calls l28_fdma_xtr_hdr_decode()
        memset(sw_dcb->data, 0, VTSS_VSTAX_HDR_SIZE);
    }
}
#endif /* defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2 */

#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2
/*****************************************************************************/
// inj_stack_hdr_move()
/*****************************************************************************/
static FDMA_INLINE void inj_stack_hdr_move(fdma_sw_dcb_t *sw_dcb)
{
    u8               *dst, *src;
    int              i;
    u8               stack_hdr[VTSS_VSTAX_HDR_SIZE];
    vtss_fdma_list_t *user_dcb = FDMA_USER_DCB(sw_dcb);

    // Ensure that there's room for the VStaX header just before the IFH.
    FDMA_ASSERT(IRQ, user_dcb->frm_ptr - sw_dcb->ifh_ptr == VTSS_FDMA_INJ_HDR_SIZE_BYTES, return;);

    // First check if this is a VStaX frame in the first place.
    u16 etype = (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS] << 8) | (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS + 1]);
    if (etype == VTSS_ETYPE_VTSS && (user_dcb->frm_ptr[SIZE_OF_TWO_MAC_ADDRS + 2] & 0x80)) {

        // It's present. First copy it into temporary space
        memcpy(stack_hdr, user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS, VTSS_VSTAX_HDR_SIZE);

        // Collapse payload by copying MAC addresses and INJ HDR VTSS_VSTAX_HDR_SIZE bytes forward.
        // We need to do it by iteration rather than memcpy() due to possible overlapping addresses
        dst = user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS + VTSS_VSTAX_HDR_SIZE - 1;
        src = user_dcb->frm_ptr + SIZE_OF_TWO_MAC_ADDRS                       - 1;

        for (i = VTSS_FDMA_INJ_HDR_SIZE_BYTES + SIZE_OF_TWO_MAC_ADDRS - 1; i >= 0; i--) {
            *(dst--) = *(src--);
        }

        // Now that we've moved the IFH VTSS_VSTAX_HDR_SIZE bytes ahead, there's room for the stack header at the beginning of the data area.
        memcpy(sw_dcb->data, stack_hdr, VTSS_VSTAX_HDR_SIZE);

        // Adjust length, ifh_ptr, and frm_ptr
        sw_dcb->ifh_ptr   += VTSS_VSTAX_HDR_SIZE;
        user_dcb->frm_ptr += VTSS_VSTAX_HDR_SIZE;
        user_dcb->act_len -= VTSS_VSTAX_HDR_SIZE;

        // All this just to be forward compatible with location of VStaX header :-(
    } else {
        // It's not present. Clear start of IFH to prevent a false hit if
        // the application code possibly attempts a manual injection.
        if (sw_dcb->ifh_ptr - sw_dcb->data > 0) {
            memset(sw_dcb->data, 0, sw_dcb->ifh_ptr - sw_dcb->data);
        }
    }
}
#endif /* defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2 */

/*****************************************************************************/
// xtr_irq_block_done()
/*****************************************************************************/
static FDMA_INLINE BOOL xtr_irq_block_done(vtss_state_t *const vstate, void *const cntxt, vtss_fdma_ch_t xtr_ch)
{
    int                    xtr_cnt_dwords, total_frm_size_bytes_excl_ifh, bytes_valid_in_last_word, drop_it;
    fdma_ch_state_t        *ch_state;
    fdma_hw_dcb_t          *hw_dcb;
    vtss_fdma_list_t       *new_list, *new_head, *temp_list;
    u32                    ctl1, dstat;
    BOOL                   sof, eof;
    fdma_state_t           *state = &vstate->fdma_state;
    VTSS_OS_TIMESTAMP_TYPE sw_tstamp;
#if VTSS_OPT_FDMA_VER >= 2
    u32                    offset_into_data = VTSS_FDMA_HDR_SIZE_BYTES - VTSS_FDMA_XTR_HDR_SIZE_BYTES;
#else
    u32                    offset_into_data = 0;
#endif /* VTSS_OPT_FDMA_VER */
#if VTSS_OPT_FDMA_DEBUG
    int                    debug_sum;
#endif /* VTSS_OPT_FDMA_DEBUG */

    VTSS_NG(VTSS_TRACE_GROUP_FDMA_IRQ, "xtr_irq_block_done(ch = %d)\n", xtr_ch);
    FDMA_ASSERT(IRQ, xtr_ch >= 0 && xtr_ch < VTSS_FDMA_CH_CNT,    return FALSE;);
    ch_state = &state->fdma_ch_state[xtr_ch];
    FDMA_ASSERT(IRQ, ch_state->status != FDMA_CH_STATUS_DISABLED, return FALSE;);
    FDMA_ASSERT(IRQ, ch_state->usage  == VTSS_FDMA_CH_USAGE_XTR,  return FALSE;);

    // Iterate over all DCBs and stop when we reach one with the DONE bit cleared.
    while (ch_state->cur_head) {
        hw_dcb = FDMA_HW_DCB(ch_state->cur_head);

        // Before dereferencing the DCB, we must invalidate the cache line(s).
        VTSS_OS_DCACHE_INVALIDATE(hw_dcb, VTSS_FDMA_DCB_SIZE_BYTES);
        VTSS_OS_REORDER_BARRIER();

        // The order that the GPDMA writes back status fields is: First write back hw_dcb->ctl1, then write back hw_dcb->dstat.
        // This means that we have to check DSTAT's MSbit before the CTL1.DONE bit.
        // DSTAT's MSbit was set by l28_fdma_xtr_dcb_init() and will be overwritten by whatever the GPDMA reads from
        // VTSS_ICPU_CFG_GPDMA_FDMA_XTR_LAST_CHUNK_STAT[xtr_ch]. This particular register's MSbit is always 0.
        dstat = BUS_TO_CPU(hw_dcb->dstat);

        if ((dstat & 0x80000000) != 0) {
            // The GPDMA has not written this register yet, so the frame is not valid.
            break;
        }

#if VTSS_OPT_FDMA_VER <= 2
        state->fdma_stats.rx_bufs_used[ch_state->phys_qu]++;
#else
        state->fdma_stats.dcbs_used[xtr_ch]++;
#endif /* VTSS_OPT_FDMA_VER */

        ctl1 = BUS_TO_CPU(hw_dcb->ctl1);

        // When DSTAT[31] == 0 then CTL1.DONE (bit 12) must be 1 as well.
        FDMA_ASSERT(IRQ, ctl1 & VTSS_BIT(12), VTSS_EG(VTSS_TRACE_GROUP_FDMA_IRQ, "ch=%d, ctl1=0x%08x", xtr_ch, ctl1); return FALSE;);

        // Bits 11:0 of dcb->ctl1 indicate the number of 32-bit words
        // extracted to the data area, so that's what we invalidate there.
        xtr_cnt_dwords = ctl1 & 0xFFF;
        VTSS_OS_DCACHE_INVALIDATE(ch_state->cur_head->data + offset_into_data, 4 * xtr_cnt_dwords);
        VTSS_OS_REORDER_BARRIER();

        // We don't expect zero or more than alloc_len bytes to having been extracted
        // to the data area.
        FDMA_ASSERT(IRQ, xtr_cnt_dwords > 0 && 4UL * xtr_cnt_dwords <= ch_state->cur_head->alloc_len - offset_into_data,
            VTSS_EG(VTSS_TRACE_GROUP_FDMA_IRQ,
            "ch=%d, xtr_cnt_dwords=%d, alloc_len=%d, offset_into_data=%d",
            xtr_ch, xtr_cnt_dwords, ch_state->cur_head->alloc_len, offset_into_data); return FALSE;);

        // Check that the queue that the FDMA thinks it extracted from matches the
        // queue that software thinks it extracts from.
        dstat = BUS_TO_CPU(hw_dcb->dstat);
        FDMA_ASSERT(IRQ, ((dstat >> 4) & (VTSS_PACKET_RX_QUEUE_CNT - 1)) == ch_state->phys_qu, return FALSE;);

        sof = (dstat & 0x1) != 0;
        eof = (dstat & 0x2) != 0;

        if (sof) {
            ch_state->cur_head->ifh_ptr = ch_state->cur_head->data    + offset_into_data;
            FDMA_USER_DCB(ch_state->cur_head)->frm_ptr = ch_state->cur_head->ifh_ptr + VTSS_FDMA_XTR_HDR_SIZE_BYTES;
        } else {
            FDMA_USER_DCB(ch_state->cur_head)->frm_ptr = ch_state->cur_head->ifh_ptr = ch_state->cur_head->data + offset_into_data;
        }

        // If we're receiving a jumbo frame (or a frame that is larger
        // than what we've reserved of space in the data area), we may
        // not have received it all. If the EOF flag in the DCB is not
        // set, this is the case, and we need to receive the remainder of
        // the frame before passing it on.
        if (eof) {
            // EOF reached. Pass the frame on to the application.

            VTSS_OS_TIMESTAMP(&sw_tstamp);

            // Terminate the current list item before passing it on, but
            // save a reference to the new head.
            new_head = FDMA_USER_DCB(ch_state->cur_head)->next;
            FDMA_USER_DCB(ch_state->cur_head)->next = NULL;

            if (!ch_state->pend_head) {
                // The frame is not part of a jumbo frame, so we
                // expect the SOF flag to be set in the DCB
                // GNATS #5299: If the frame fills up a DCB, the SOF flag is not set.
#if defined(VTSS_FDMA_IGNORE_GNATS_5299)
                FDMA_ASSERT(IRQ, sof, return FALSE;);
#endif /* VTSS_FDMA_IGNORE_GNATS_5299 */

                // For further, unified processing, we move the current DCB to the pending list.
                ch_state->pend_head = ch_state->cur_head;
            } else {
                // The frame is part of a jumbo frame.
                // In that case, the SOF flag can't be set, because the SOF is
                // already received and moved to the pending list.
                FDMA_ASSERT(IRQ, !sof, return FALSE;);

                // Attach the current list item to the end of the pending list.
                temp_list = FDMA_USER_DCB(ch_state->pend_head);
                while (temp_list->next) {
                    temp_list = temp_list->next;
                }
                temp_list->next = FDMA_USER_DCB(ch_state->cur_head);
            }

            // Update the current fragment's actual length. For the EOF
            // DCB this need not be a multiple of 4, as is the case for
            // non-EOF DCBs. We use the IFH's total frame length to figure
            // out how many bytes are valid in this last fragment.
            total_frm_size_bytes_excl_ifh = (BUS_TO_CPU(*(u32 *)(ch_state->pend_head->data + offset_into_data)) >> 16) & 0x3FFF;
            bytes_valid_in_last_word = total_frm_size_bytes_excl_ifh & 0x3;
            if (!bytes_valid_in_last_word) {
                bytes_valid_in_last_word = 4;
            }
            FDMA_USER_DCB(ch_state->cur_head)->act_len = 4 * xtr_cnt_dwords - (4 - bytes_valid_in_last_word);

#if VTSS_OPT_FDMA_DEBUG
            // Before sending off the frame, check that all the fragment
            // sizes sum up to the frame length indicated in the IFH.
            temp_list = FDMA_USER_DCB(ch_state->pend_head);
            debug_sum = 0;
            while (temp_list) {
                debug_sum += temp_list->act_len;
                temp_list = temp_list->next;
            }
            FDMA_ASSERT(IRQ, total_frm_size_bytes_excl_ifh + VTSS_FDMA_XTR_HDR_SIZE_BYTES + 4 * ch_state->start_gap + 4 * ch_state->end_gap == debug_sum, return FALSE;);
#endif /* VTSS_FDMA_DEBUG */

            drop_it = 0;
#if !defined(VTSS_FDMA_IGNORE_GNATS_5418)
            // GNATS #5418 says: When transmitting a frame from CPU PM using DO_ANALYZE, then the frame
            // may be looped back to the CPU, even if the ANA::SRCMASKS[CPU_PM] doesn't include the CPU
            // PM itself. The reason is that the SRCMASKS are checked before it is determined whether
            // the frame should be copied to the CPU or not (which is typically the case for BPDUs and
            // broadcast frames).
            // The work-around is to drop all frames with IFH.src_port == VTSS_CPU_PM_NUMBER. But we only
            // do this if VTSS_FDMA_IGNORE_GNATS_5418 is not defined. If it's defined, we also forward that
            // type of frames to xtr_cb().
            {
                u32 *ifh_ptr = (u32 *)(ch_state->pend_head->data + offset_into_data);
                u32 ifh0 = BUS_TO_CPU(ifh_ptr[0]);
                u32 src_port = (ifh0 >> (VTSS_FDMA_IFH_F_SRC_PORT_FPOS - 32)) & VTSS_BITMASK(VTSS_FDMA_IFH_F_SRC_PORT_FLEN);
                if (src_port == VTSS_CPU_PM_NUMBER) {
                    drop_it = 1;
                }
            }
#endif /* !defined(VTSS_FDMA_IGNORE_GNATS_5418) */

#if VTSS_OPT_FDMA_VER <= 2
            // Don't count the frame if it's dropped due to an inadvertent CPU PM loopback.
            if (!drop_it) {
                state->fdma_stats.rx_packets[ch_state->phys_qu]++;
                // Don't include IFH in packet byte counter
                state->fdma_stats.rx_bytes[ch_state->phys_qu] += total_frm_size_bytes_excl_ifh;
            } else {
                state->fdma_stats.rx_dropped++;
            }
#else
            // Don't count the frame if it's dropped due to an inadvertent CPU PM loopback.
            if (!drop_it) {
                state->fdma_stats.packets[xtr_ch]++;
                // Don't include IFH in packet byte counter
                state->fdma_stats.bytes[xtr_ch] += total_frm_size_bytes_excl_ifh;
            } else {
                state->fdma_stats.xtr_qu_drops[ch_state->phys_qu][0]++;
            }
#endif /* VTSS_OPT_FDMA_VER */

            if (drop_it) {
                // Don't send to xtr_cb(), but re-cycle the DCBs.
                new_list = FDMA_USER_DCB(ch_state->pend_head);
            } else {
#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2
                // Gotta move a possible VStaX header from the frame itself into the IFH, in which there's room for it.
                xtr_stack_hdr_move(ch_state->pend_head);
#endif /* defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2 */
#if VTSS_OPT_FDMA_VER >= 3
                if (vtss_fdma_cmn_xtr_hdr_decode(vstate, ch_state->pend_head, 0, ch_state->phys_qu, &sw_tstamp) == VTSS_RC_OK) {
                    new_list = state->fdma_cfg.rx_cb(cntxt, FDMA_USER_DCB(ch_state->pend_head));
                } else {
                    // Re-cycle
                    new_list = FDMA_USER_DCB(ch_state->pend_head);
                }
#else
                ch_state->pend_head->timestamp = sw_tstamp;
                new_list = ch_state->xtr_cb(cntxt, ch_state->pend_head, ch_state->phys_qu + VTSS_PACKET_RX_QUEUE_START);
#endif /* VTSS_OPT_FDMA_VER */
            }

            // NULLify the pending list, since there're no more pending fragments.
            ch_state->pend_head = NULL;

            // Return the DCBs (if any) to the extraction channel. Don't restart the channel (that's handled in xtr_irq_tfr_done()).
            (void)vtss_fdma_cmn_dcb_release(vstate, xtr_ch, FDMA_SW_DCB(new_list), TRUE /* thread safe */, TRUE /* connect */, FALSE /* don't restart channel */);
        } else {
            // The EOF is not reached yet, so there wasn't room for the whole frame
            // in the allocated data area of this DCB. Move the DCB to the pending list.

            // Update the current fragment's actual length from the
            // xtr_cnt_dwords. We know that there's always a multiple
            // of 4 bytes extracted to a fragment.
            FDMA_USER_DCB(ch_state->cur_head)->act_len = 4 * xtr_cnt_dwords;

            // Save a reference to the new head.
            new_head = FDMA_USER_DCB(ch_state->cur_head)->next;

            // Clear the next entry, so that we can accommodate the case where the
            // FDMA's DCB list exhausts in the middle of a jumbo frame, causing
            // xtr_irq_tfr_done() to restart the channel with a new list of DCBs.
            // If we didn't always clear the pend_head->next we wouldn't know where
            // to attach the remainder of jumbo frames.
            FDMA_USER_DCB(ch_state->cur_head)->next = NULL;

            if(!ch_state->pend_head) {
                // This is the first part of the frame.
                FDMA_ASSERT(IRQ, sof, VTSS_EG_HEX(VTSS_TRACE_GROUP_FDMA_IRQ, (u8 *)ch_state, sizeof(*ch_state)); VTSS_EG_HEX(VTSS_TRACE_GROUP_FDMA_IRQ, (u8 *)hw_dcb, sizeof(*hw_dcb))); // The SOF flag must be set. Continue execution if it isn't.
                ch_state->pend_head = ch_state->cur_head;
            } else {
                // This is not the first and not the last part of the frame.
                FDMA_ASSERT(IRQ, !sof, VTSS_EG_HEX(VTSS_TRACE_GROUP_FDMA_IRQ, (u8 *)ch_state, sizeof(*ch_state)); VTSS_EG_HEX(VTSS_TRACE_GROUP_FDMA_IRQ, (u8 *)hw_dcb, sizeof(*hw_dcb))); // The SOF flag must be cleared. Continue execution if it isn't.

                // We need to attach it to the last item in the pend_head list.
                temp_list = FDMA_USER_DCB(ch_state->pend_head);
                while(temp_list->next) {
                    temp_list = temp_list->next;
                }
                temp_list->next = FDMA_USER_DCB(ch_state->cur_head);
            }
        }

        // Advance the head of the "in progress" list
        ch_state->cur_head = FDMA_SW_DCB(new_head);
    }

    // Return non-zero if the active head is now NULL.
    return ch_state->cur_head == NULL;
}

/*****************************************************************************/
// xtr_irq_tfr_done()
// We need to restart the channel.
/*****************************************************************************/
static FDMA_INLINE void xtr_irq_tfr_done(vtss_state_t *const vstate, vtss_fdma_ch_t xtr_ch)
{
    fdma_ch_state_t *ch_state = &vstate->fdma_state.fdma_ch_state[xtr_ch];

    FDMA_ASSERT(IRQ, ch_state->status != FDMA_CH_STATUS_DISABLED, return;);
    FDMA_ASSERT(IRQ, ch_state->usage  == VTSS_FDMA_CH_USAGE_XTR,  return;);
    VTSS_NG(VTSS_TRACE_GROUP_FDMA_IRQ, "xtr_irq_tfr_done(ch = %d). New DCBs available: %s\n", xtr_ch, ch_state->free_head?"yes":"no");

    // The following function performs all the necessary checks for the restart to take effect.
    vtss_fdma_cmn_xtr_restart_ch(vstate, xtr_ch);
}

#if VTSS_OPT_FDMA_VER >= 3
/****************************************************************************/
// inj_manual_data()
// Write one 32-bit word to super priority queue
/****************************************************************************/
static FDMA_INLINE BOOL inj_manual_data(vtss_state_t *const vstate, int port, u32 value)
{
    u32 read_attempt = 0;
    u32 miscstat;

    VCOREII_WR(vstate, VTSS_DEV_CPUTXDAT(port), value);

    // Poll the port until written or timeout
    do {
        miscstat = VCOREII_RD(vstate, VTSS_DEV_MISCSTAT(port));
        if (!(miscstat & VTSS_F_MISCSTAT_CPU_TX_DATA_PENDING)) {
            return TRUE;
        }
    } while (read_attempt++ < 100);

    // Timeout. Cancel transmission
    VCOREII_WR(vstate, VTSS_DEV_MISCFIFO(port), VTSS_F_MISCFIFO_CLEAR_STICKY_BITS | VTSS_F_MISCFIFO_REWIND_CPU_TX);
    //lint --e{506}
    FDMA_ASSERT(IRQ, FALSE, ;);
    return FALSE;
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if VTSS_OPT_FDMA_VER >= 3
/****************************************************************************/
// BYTE_SWAP()
/****************************************************************************/
static inline u32 BYTE_SWAP(u8 *p)
{
    return (((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3]);
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if VTSS_OPT_FDMA_VER >= 3
/****************************************************************************/
// inj_frm_manual()
// Transmit the frame word by word and check super-priority status for each
// word.
// Returns TRUE on success, FALSE on failure (timeout).
/****************************************************************************/
static BOOL inj_frm_manual(vtss_state_t *const vstate, fdma_sw_dcb_t *sw_dcb)
{
    u32              miscstat, port, i, len_dwords, len_bytes, num_used_bytes_from_dcb, bytes_in_dcb;
    u32              *data;
    fdma_sw_dcb_t    *l;
    vtss_fdma_list_t *user_dcb;

    VTSS_IG(VTSS_TRACE_GROUP_FDMA_IRQ, "Resorting to manual injection. active_cnt[%u] = %u", sw_dcb->phys_port, vstate->fdma_state.fdma_port_state[sw_dcb->phys_port].active_cnt);

    // This (currently) only works with front ports.
    FDMA_ASSERT(IRQ, sw_dcb && sw_dcb->phys_port < VTSS_PHYS_PORT_CNT && vstate->fdma_state.fdma_port_state[sw_dcb->phys_port].active_cnt == 0, return FALSE;);

    // Get the full length of the frame (excl. IFH and CMD, but incl. FCS).
    l = sw_dcb;
    len_bytes = 4 /* FCS */;
    while (l) {
        user_dcb = FDMA_USER_DCB(l);
        len_bytes += user_dcb->act_len;

        // This (currently) only works with 32-bit aligned data.
        FDMA_ASSERT(IRQ, ((u32)(       l->data)    & 0x3) == 0, return FALSE;);
        FDMA_ASSERT(IRQ, ((u32)(       l->ifh_ptr) & 0x3) == 0, return FALSE;);
        FDMA_ASSERT(IRQ, ((u32)(user_dcb->frm_ptr) & 0x3) == 0, return FALSE;);
        if (user_dcb->next) {
            // This (currently) only works if all but the EOF-DCBs' length is a multiple of 4.
            FDMA_ASSERT(IRQ, ((user_dcb->act_len) & 0x3) == 0, return FALSE;);
        }

        l = FDMA_SW_DCB(user_dcb->next);
    }

    // Get the number of dwords used by this frame (excl. IFH and CMD fields, but incl. FCS).
    len_dwords = (len_bytes + 3) / 4;

    port = sw_dcb->phys_port;
    data = (u32 *)sw_dcb->ifh_ptr;

    miscstat = VCOREII_RD(vstate, VTSS_DEV_MISCSTAT(port));
    if ((miscstat & (VTSS_F_MISCSTAT_CPU_TX_QUEUE_FULL | VTSS_F_MISCSTAT_CPU_TX_DATA_PENDING | VTSS_F_MISCSTAT_CPU_TX_DATA_OVERFLOW)) != 0) {
        // Clean up
        VCOREII_WR(vstate, VTSS_DEV_MISCFIFO(port), VTSS_F_MISCFIFO_CLEAR_STICKY_BITS | VTSS_F_MISCFIFO_REWIND_CPU_TX);
        VTSS_EG(VTSS_TRACE_GROUP_FDMA_IRQ, "miscstat = %u, active_cnt[%u] = %u", miscstat, port, vstate->fdma_state.fdma_port_state[port].active_cnt);
        return FALSE;
    }

    // Send IFH. We don't bother to re-compute it ourselves, so we use that from the packet. It's stored in packet->data[7:0]
    for (i = 0; i < 2; i++) {
        // Data in IFH part of frame is already in big endian, so don't call BYTE_SWAP()
        if (!inj_manual_data(vstate, port, *data)) {
            return FALSE;
        }
        data++;
    }

    // Let data point to first byte of frame, i.e. skip the CMD field.
    data++;

    l = sw_dcb;
    user_dcb = FDMA_USER_DCB(sw_dcb);
    num_used_bytes_from_dcb = 0;
    bytes_in_dcb = user_dcb->act_len + (user_dcb->next ? 0 : 4 /* FCS */);

    // Send actual frame data
    for (i = 0; i < len_dwords; i++) {
        if (num_used_bytes_from_dcb >= bytes_in_dcb) {
            user_dcb = user_dcb->next;
            l = FDMA_SW_DCB(user_dcb);
            if (!l) {
                VTSS_EG(VTSS_TRACE_GROUP_FDMA_IRQ, "num_used_bytes_from_dcb = %u, bytes_in_dcb = %u, next = %p, act_len = %u", num_used_bytes_from_dcb, bytes_in_dcb, FDMA_USER_DCB(sw_dcb)->next, FDMA_USER_DCB(sw_dcb)->act_len);
                return FALSE;
            }
            data = (u32 *)user_dcb->frm_ptr;
            num_used_bytes_from_dcb = 0;
            bytes_in_dcb = user_dcb->act_len + (user_dcb->next ? 0 : 4 /* FCS */);
        }
        // BYTE_SWAP() works both for converting to big and to little endian. In our case we need to convert to big.
        if (!inj_manual_data(vstate, port, BYTE_SWAP((u8 *)data))) {
            return FALSE;
        }
        data++;
        num_used_bytes_from_dcb += 4;
    }

    if (len_dwords & 1) {
        if (!inj_manual_data(vstate, port, 0)) {
            return FALSE;
        }
    }

    // Go baby.
    VCOREII_WR(vstate, VTSS_DEV_MISCFIFO(port), VTSS_F_MISCFIFO_CPU_TX);

    return TRUE;
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

/*****************************************************************************/
// inj_irq_tfr_done()
/*****************************************************************************/
static FDMA_INLINE void inj_irq_tfr_done(vtss_state_t *const vstate, void *const cntxt, vtss_fdma_ch_t ch)
{
    fdma_state_t      *state = &vstate->fdma_state;
    fdma_ch_state_t   *ch_state;
    fdma_port_state_t *port_state;
    int               port;
    BOOL              dropped, do_retransmit, possible_retransmit = FALSE;
    vtss_fdma_list_t  *sof_l, *eof_l;
    u32               miscstat;
    fdma_hw_dcb_t     *hw_dcb;
#if VTSS_OPT_FDMA_VER <= 2
    int               orig_port;
#endif /* VTSS_OPT_FDMA_VER <= 2 */

    VTSS_NG(VTSS_TRACE_GROUP_FDMA_IRQ, "inj_irq_tfr_done(ch = %d)\n", ch);

    FDMA_ASSERT(IRQ, ch >= 0 && ch < VTSS_FDMA_CH_CNT,            return;);
    ch_state=&state->fdma_ch_state[ch];
    FDMA_ASSERT(IRQ, ch_state->status != FDMA_CH_STATUS_DISABLED, return;);
    FDMA_ASSERT(IRQ, ch_state->usage  == VTSS_FDMA_CH_USAGE_INJ,  return;);
    FDMA_ASSERT(IRQ, ch_state->cur_head,                          return;);
    sof_l = eof_l = FDMA_USER_DCB(ch_state->cur_head);

    // We need to traverse the DCBs until we find a DCB with the hw_dcb->llp
    // location being NULL, which is the latest injected frame's EOF. We get called
    // once per frame that has been injected, so it cannot happen that the DMA
    // controller goes beyond that point in the list.
    while (1) {
        FDMA_ASSERT(IRQ, eof_l, return;);

        // Check that the DMA controller indeed has injected the current block.
        // This is conveyed through the hw_dcb->ctl1.DONE bit.
        // Since the DMA controller writes this bit, we must invalidate the cache
        // for that position before reading it. It is the only part of the DCB
        // that the DMA controller writes back, so it's the only part that we need
        // to invalidate. Also, since we only use it in an assert, we need only
        // invalidate it in VTSS_OPT_FDMA_DEBUG mode.
        hw_dcb = FDMA_HW_DCB(FDMA_SW_DCB(eof_l));

#if VTSS_OPT_FDMA_DEBUG
        VTSS_OS_DCACHE_INVALIDATE(&hw_dcb->ctl1, 4);
        VTSS_OS_REORDER_BARRIER();
        FDMA_ASSERT(IRQ, BUS_TO_CPU(hw_dcb->ctl1) & 0x1000, return;);
#endif /* VTSS_OPT_FDMA_DEBUG */

        if (hw_dcb->llp) {
            eof_l = eof_l->next;
        } else {
            break;
        }
    }

    // Check if the relevant port is in flow control mode. If so, we haven't yet
    // transmitted the frame, because we need to read the queue status prior to
    // setting the CPU_TX flag.
    port = FDMA_SW_DCB(sof_l)->phys_port;
    port_state = &state->fdma_port_state[port];

#if VTSS_OPT_FDMA_VER <= 2
    orig_port = port;
#endif /* VTSS_OPT_FDMA_VER <= 2 */

    // The port is one less loaded now, i.e. this channel is currently not injecting
    // to this port anymore (others may, however).
    FDMA_ASSERT(IRQ, port_state->active_cnt > 0, return;);
    port_state->active_cnt--;

    if (port_state->cfg_tx_flow_ctrl) {
        miscstat = VCOREII_RD(vstate, VTSS_DEV_MISCSTAT(port));
        if (miscstat & (VTSS_F_MISCSTAT_CPU_TX_QUEUE_FULL | VTSS_F_MISCSTAT_CPU_TX_DATA_PENDING | VTSS_F_MISCSTAT_CPU_TX_DATA_OVERFLOW)) {
            // An overflow occurred, or the last data is not transferred to the RAM, or the RAM is full.
            // Rewind the current frame while clearing sticky bits
            VCOREII_WR(vstate, VTSS_DEV_MISCFIFO(port), VTSS_F_MISCFIFO_CLEAR_STICKY_BITS | VTSS_F_MISCFIFO_REWIND_CPU_TX);
            possible_retransmit = TRUE;
        } else {
            // No overflow occurred. Transmit the current frame.
            VCOREII_WR(vstate, VTSS_DEV_MISCFIFO(port), VTSS_F_MISCFIFO_CPU_TX);
        }
    }

    dropped       = possible_retransmit && port_state->state_retransmit_cnt == 0;
    do_retransmit = possible_retransmit && port_state->state_retransmit_cnt != 0;

    // If an error occurred, (possible_retransmit == TRUE), then check the retransmit_cnt to
    // see if we need to retransmit the frame or should drop it.
    if (do_retransmit) {
        // Do attempt a retransmit
        port_state->state_retransmit_cnt--;
    } else {
        // If we get here, the previous frame was either dropped or sent
        // successfully, but at least shouldn't be retransmitted.
        //
        // eof_l points to the EOF list item.
        // Terminate it before passing it on to the callback function,
        // but keep a pointer to the next in the list, which is the one
        // we need to reactive the channel for if non-NULL.
        ch_state->cur_head = FDMA_SW_DCB(eof_l->next);
        eof_l->next = NULL;
    }

#if VTSS_OPT_FDMA_VER >= 3
    if (dropped) {
        // Resort to manual transmission before starting the channel
        // again with the frames.
        dropped = !inj_frm_manual(vstate, FDMA_SW_DCB(sof_l));
        FDMA_ASSERT(IRQ, !dropped, ;);
    }
#endif

#if VTSS_OPT_FDMA_VER <= 2
    VTSS_OS_TIMESTAMP(&sof_l->timestamp);
#else
    VTSS_OS_TIMESTAMP(&sof_l->sw_tstamp);
#endif /* VTSS_OPT_FDMA_VER */

    if (ch_state->cur_head) {
        // More frames to go. Re-start the channel.
        // Find the port number to inject the frame to. If the port is
        // different from the port we just injected frames to, we need to
        // re-configure the DMA controller. This is also the reason why we
        // only send one frame at a time before being interrupted.
        port = ch_state->cur_head->phys_port;
        VTSS_DG(VTSS_TRACE_GROUP_FDMA_IRQ, "inj_irq_tfr_done(ch = %d). Re-starting channel", ch);

        if (!do_retransmit) {
            state->fdma_port_state[port].state_retransmit_cnt = state->fdma_port_state[port].cfg_retransmit_cnt; // Re-initialize the state retransmit count for the new frame.
        }

        // Restart the channel if allowed to.
        if (!state->fdma_port_state[port].suspended) {
            state->fdma_port_state[port].active_cnt++;
            // The inj_ch_init() function changes ch_state->phys_port, which holds the currently configured port.
            inj_ch_init(vstate, FDMA_HW_DCB(ch_state->cur_head), ch, port);
            enable_ch(vstate, ch);
        }
    } else {
        // No more frames. Also reset the tail.
        ch_state->cur_tail = NULL;
    }

    if (!do_retransmit) {
#if VTSS_OPT_FDMA_VER <= 2
        if (dropped) {
            state->fdma_stats.tx_dropped[orig_port]++;
        } else {
            state->fdma_stats.tx_packets[ch]++;
        }
#else
        // Can't count tx_dropped frames.
        if (!dropped) {
            state->fdma_stats.packets[ch]++;
        }
#endif /* VTSS_OPT_FDMA_VER */

#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2
        // Gotta move a possible VStaX header back from the frame itself into the IFH before calling back the
        // application, because it expects the DMAC to be located at sof_l->data_ptr + VTSS_FDMA_HDR_SIZE_BYTES.
        // Some modules may wish to retransmit the exact same frame, so in order to revert the insertion of the
        // VStaX header position, this is required.
        // Furthermore, if @dropped is TRUE, the application may want to revert to manual frame injection.
        // In this case, the application must know the position of the VStaX header.
        inj_stack_hdr_move(FDMA_SW_DCB(sof_l));
#endif /* defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2 */

#if VTSS_OPT_FDMA_VER <= 2
        // Callback the Tx handler unless this is a simple retransmit
        FDMA_ASSERT(IRQ, sof_l->inj_post_cb, return;);
        sof_l->inj_post_cb(cntxt, sof_l, ch, dropped);
#else
        vstate->fdma_state.fdma_cfg.tx_done_cb(cntxt, sof_l, dropped ? VTSS_RC_ERROR : VTSS_RC_OK);

        // Release the injection DCBs.
        (void)vtss_fdma_cmn_dcb_release(vstate, 0 /* doesn't matter in FDMA v3+ */, FDMA_SW_DCB(sof_l), TRUE /* thread safe */, FALSE /* only used for XTR DCBs */, TRUE /* only used for XTR DCBs */);
#endif /* VTSS_OPT_FDMA_VER */
    }
}

/*****************************************************************************/
// xtr_start_ch()
/*****************************************************************************/
static void xtr_start_ch(vtss_state_t *const vstate, vtss_fdma_ch_t xtr_ch)
{
    fdma_ch_state_t *ch_state = &vstate->fdma_state.fdma_ch_state[xtr_ch];
    fdma_hw_dcb_t   *hw_dcb = FDMA_HW_DCB(ch_state->cur_head);

    // Reset the extraction queue by reading this register.
    // Since the queue system RAM is 64 bits, we must read an even number of times
    // to make the state machine end up in a valid state before the FDMA starts
    // reading real data out of it. Hopefully the FDMA also reads an even number
    // of times.
    (void)VCOREII_RD(vstate, VTSS_XTR_QU_RST(ch_state->phys_qu));
    (void)VCOREII_RD(vstate, VTSS_XTR_QU_RST(ch_state->phys_qu));

    VTSS_OS_REORDER_BARRIER();

    // Make sure the FDMA gets in a well-defined state.
    VCOREII_WR(vstate, GPDMA_FDMA_XTR_CFG(xtr_ch), VTSS_F_XTR_INIT_SHOT);

    VTSS_OS_REORDER_BARRIER();

    VCOREII_WR(vstate, VTSS_CH_LLP(xtr_ch), VTSS_OS_VIRT_TO_PHYS(hw_dcb));
    VCOREII_WR(vstate, GPDMA_FDMA_CH_CFG_0(xtr_ch), VTSS_F_MODULE(4) | VTSS_F_SUB_MODULE(0) | VTSS_F_DATA_ADDR(0)); // Sub-module must be 0, since the FDMA computes the sub-module itself from the queue number.

    // Whether we're in little or big endian mode we need to swap the data bytes
    // between DDR SDRAM and the SwC, because the SwC has them stored in big
    // endian format and we need them stored as a stream of bytes in the DDR SDRAM.
    // Since there's no endianness awareness between the DMA controller and the
    // DDR SDRAM there is no difference between the storing of data whether the
    // CPU is in little or big endian mode.
    VCOREII_WR(vstate, VTSS_GPDMA_FDMA_COMMON_CFG, VTSS_F_DATA_BYTE_SWAP);

    // Usage=extraction, queue=xtr_qu,
    VCOREII_WR(vstate, GPDMA_FDMA_CH_CFG_1(xtr_ch), VTSS_F_FC_XTR_ENA(VTSS_BIT(ch_state->phys_qu))); // USAGE=extraction and FC_XTR_ENA
    VCOREII_WR(vstate, VTSS_CH_LLP(xtr_ch),  VTSS_OS_VIRT_TO_PHYS(hw_dcb));
    VCOREII_WR(vstate, VTSS_CH_CTL0(xtr_ch), VTSS_F_LLP_SRC_EN  | VTSS_F_LLP_DST_EN); // We just need to enable LLP_SRC_EN and LLP_DST_EN here, since a new descriptor is loaded right after.
    VCOREII_WR(vstate, VTSS_CH_CFG0(xtr_ch), VTSS_F_HS_SEL_DST  | VTSS_F_CH_PRIOR(ch_state->prio)); // HS_SEL_SRC=Hardware Initiated, HS_SEL_DST=1=Software Initiated, keep intrinsic priority
    VCOREII_WR(vstate, VTSS_CH_CFG1(xtr_ch), VTSS_F_SRC_PER(xtr_ch) | VTSS_F_DS_UPD_EN | VTSS_F_PROTCTL(1) | VTSS_F_FCMODE); // SRC_PER=channel, DS_UPD_EN=1, PROTCTL=1, FCMODE=1

    // Set-up start- and end-gaps
    VCOREII_WR(vstate, GPDMA_FDMA_XTR_CFG(xtr_ch), VTSS_F_XTR_END_GAP(ch_state->end_gap) | VTSS_F_XTR_START_GAP(ch_state->start_gap));

    // With the DS_UPD_EN bit set in CFG1, we must also set the address from which the destination update status
    // is fetched by the DMA controller. The CFG::XTR_LAST_CHUNK_STAT provides the needed information (EOF, SOF, and channel ID).
    // Hmm. I don't know how to get hold of the address of the XTR_LAST_CHUNK_STAT, so I hardcode it.
    VCOREII_WR(vstate, VTSS_CH_DSTATAR(xtr_ch), FDMA_LAST_CHUNK_PHYS_ADDR(xtr_ch));

    // We enable block interrupts for extraction channels, so that we get an interrupt every time a DCB has been filled with
    // data. If we used Transfer Done interrupts, we wouldn't get an interrupt until the list of DCBs was exhausted.
    // We also, on the other hand enable transfer done interrupts. This is needed in order to restart a channel if it happens
    // that the list of DCBs gets exhausted, which may occur if the FDMA transfers packets faster than the operating system
    // can take them off.
    enable_block_done_intr(vstate, xtr_ch);
    enable_tfr_done_intr(vstate, xtr_ch);

    // Clear pending interrupts
    clear_pending_block_done_intr(vstate, VTSS_BIT(xtr_ch));
    clear_pending_tfr_done_intr  (vstate, VTSS_BIT(xtr_ch));

    // Enable the channel
    enable_ch(vstate, xtr_ch);
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// xtr_cfg()
// To avoid OS-dependent allocation problems on the various OS's we support,
// we require, that the list[] is pre-allocated and that the data areas that
// its members point to are as well.
// Call this function per channel and queue.
// @ena:
//   If FALSE, the channel gets disabled and the remaining parameters (except for xtr_ch) are unused.
//   If TRUE,  the remaining parameters are used and the channel gets enabled.
/*****************************************************************************/
static vtss_rc xtr_cfg(      vtss_state_t           *const vstate,
                       const BOOL                   ena,
                       const vtss_fdma_ch_t         xtr_ch,
                       const vtss_packet_rx_queue_t qu,
                       const unsigned int           start_gap,
                       const unsigned int           end_gap,
                             vtss_fdma_list_t       *const list,
                             vtss_fdma_list_t       *(*xtr_cb)(void *cntxt, vtss_fdma_list_t *list, vtss_packet_rx_queue_t qu),
                       const unsigned int           prio)
{
    int              i;
    u32              phys_qu = qu - VTSS_PACKET_RX_QUEUE_START; // Get a 0-based queue number (if not already)
    fdma_state_t     *state = &vstate->fdma_state;
    fdma_ch_state_t  *ch_state = &state->fdma_ch_state[xtr_ch];

    // Valid channel?
    FDMA_ASSERT(NORMAL, xtr_ch >= 0 && xtr_ch < VTSS_FDMA_CH_CNT, return VTSS_RC_ERROR;);

    if (ena) {
        // Check parameters and the current status
        FDMA_ASSERT(NORMAL, phys_qu < VTSS_PACKET_RX_QUEUE_CNT,                                return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL, list,                                                              return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL, xtr_cb,                                                            return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL, ch_state->status == FDMA_CH_STATUS_DISABLED,                       return VTSS_RC_INV_STATE;);
        FDMA_ASSERT(NORMAL, start_gap < VTSS_BIT(VTSS_F_XTR_START_GAP_FLEN),                   return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL, end_gap   < VTSS_BIT(VTSS_F_XTR_END_GAP_FLEN),                     return VTSS_RC_ERROR;);
        FDMA_ASSERT(NORMAL, prio < VTSS_FDMA_CH_CNT,                                           return VTSS_RC_ERROR;);

        // Check that no other non-inactive, extraction channel is managing the qu.
        for(i = 0; i < VTSS_FDMA_CH_CNT; i++) {
            FDMA_ASSERT(NORMAL, state->fdma_ch_state[i].status   == FDMA_CH_STATUS_DISABLED ||
                                state->fdma_ch_state[i].usage    != VTSS_FDMA_CH_USAGE_XTR  ||
                                state->fdma_ch_state[i].phys_qu  != phys_qu, return VTSS_RC_INV_STATE;);
        }

        ch_state->usage     = VTSS_FDMA_CH_USAGE_XTR;
        ch_state->status    = FDMA_CH_STATUS_ENABLED;
        ch_state->cur_head  = list;
        ch_state->free_head = NULL; // No un-committed items.
        ch_state->cur_tail  = NULL;
        ch_state->pend_head = NULL;
        ch_state->phys_qu   = phys_qu;
        ch_state->xtr_cb    = xtr_cb;
        ch_state->start_gap = start_gap;
        ch_state->end_gap   = end_gap;
        ch_state->prio      = prio;

        // Initialize the list's DCB areas.
        VTSS_RC(vtss_fdma_cmn_dcb_release(vstate, xtr_ch, list, TRUE /* pretend it's thread safe */, FALSE /* don't connect */, FALSE /* don't restart channel */));

        xtr_start_ch(vstate, xtr_ch);
    } else {
        // Suspend the channel
        disable_ch(vstate, xtr_ch);
        ch_state->usage  = VTSS_FDMA_CH_USAGE_UNUSED;
        ch_state->status = FDMA_CH_STATUS_DISABLED;

        // Disable block and transfer interrupts for the channel. Clearing of the interrupts occur
        // if the channel gets re-enabled.
        disable_block_done_intr(vstate, xtr_ch);
        disable_tfr_done_intr(vstate, xtr_ch);
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

/*****************************************************************************/
// inj_start_ch()
/*****************************************************************************/
static void inj_start_ch(vtss_state_t *const vstate, vtss_fdma_ch_t inj_ch)
{
    // Since the same channel can be used to inject to different ports, and since we need to re-configure
    // the DMA controller if the port changes, we must get a new interrupt for every frame that has been
    // injected. Therefore we make sure that the DCB->llp is NULL for the last DCB of every
    // frame, and that the interrupt enable flag in DCB->ctl1 is 1 for the EOF DCB, only.
    // When the DMA controller has injected the EOF DCB, it will then invoke the transfer done interrupt
    // so that we can notify the callback function that the frame has been transmitted, and possibly
    // restart the controller if there're pending frames.
    enable_tfr_done_intr(vstate, inj_ch);
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// inj_cfg()
/*****************************************************************************/
static vtss_rc inj_cfg(      vtss_state_t   *const vstate,
                       const BOOL           ena,
                       const vtss_fdma_ch_t inj_ch,
                       const int            prio)
{
    fdma_ch_state_t *ch_state = &vstate->fdma_state.fdma_ch_state[inj_ch];

    // Valid channel?
    FDMA_ASSERT(NORMAL, inj_ch >= 0 && inj_ch < VTSS_FDMA_CH_CNT, return VTSS_RC_ERROR;);

    if (ena) {
        // Check parameters and the current status
        FDMA_ASSERT(NORMAL, ch_state->status == FDMA_CH_STATUS_DISABLED, return VTSS_RC_INV_STATE;);
        FDMA_ASSERT(NORMAL, prio >= 0 && prio < VTSS_FDMA_CH_CNT,        return VTSS_RC_ERROR;);

        // Whether we're in little or big endian mode we need to swap the data bytes
        // between DDR SDRAM and the SwC, because the SwC expects them in big endian
        // format, and since the data when stored in DDR SDRAM is a stream of single
        // bytes, there's no difference on their layout in memory in little and big
        // endian mode.
        VCOREII_WR(vstate, VTSS_GPDMA_FDMA_COMMON_CFG, VTSS_F_DATA_BYTE_SWAP);

        ch_state->usage     = VTSS_FDMA_CH_USAGE_INJ;
        ch_state->status    = FDMA_CH_STATUS_ENABLED;
        ch_state->cur_head  = NULL;
        ch_state->cur_tail  = NULL;
        ch_state->pend_head = NULL;
        ch_state->phys_port = (u32)-1; // Currently the channel is not configured for any front port
        ch_state->prio      = prio;

        inj_start_ch(vstate, inj_ch);

    } else {
        // Suspend the channel
        disable_ch(vstate, inj_ch);
        ch_state->usage  = VTSS_FDMA_CH_USAGE_UNUSED;
        ch_state->status = FDMA_CH_STATUS_DISABLED;
        disable_tfr_done_intr(vstate, inj_ch);
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/*****************************************************************************/
// l28_fdma_start_ch()
/*****************************************************************************/
static void l28_fdma_start_ch(vtss_state_t *const vstate, const vtss_fdma_ch_t ch)
{
    if (vstate->fdma_state.fdma_ch_state[ch].usage == VTSS_FDMA_CH_USAGE_XTR) {
        xtr_start_ch(vstate, ch);
    } else {
        inj_start_ch(vstate, ch);
    }
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

/*****************************************************************************/
// ch_state_defaults()
/*****************************************************************************/
static FDMA_INLINE void ch_state_defaults(vtss_state_t *const vstate)
{
#if VTSS_OPT_FDMA_VER >= 3
    fdma_ch_state_t    *ch_state;
    vtss_fdma_ch_t      ch, xtr_ch, inj_ch;
    vtss_phys_port_no_t phys_port;

    // Allocation:
    // 0: Tx switched
    // 1: Rx qu #0
    // 2: Rx qu #1
    // 3: Rx qu #2
    // 4: Rx qu #3
    // 5: Tx port-specific
    // 6: Tx stack A
    // 7: Tx stack B

    // The FDMA API makes its own channel setup in later FDMA driver versions.
    for (xtr_ch = 1; xtr_ch <= 4; xtr_ch++) {
        ch_state = &vstate->fdma_state.fdma_ch_state[xtr_ch];
        ch_state->usage    = VTSS_FDMA_CH_USAGE_XTR;
        ch_state->phys_qu  = xtr_ch - 1;
        ch_state->prio     = xtr_ch - 1;
    }

    for (ch = 4; ch <= 7; ch++) {
        inj_ch = ch == 4 ? 0 : ch;
        ch_state = &vstate->fdma_state.fdma_ch_state[inj_ch];
        ch_state->usage    = VTSS_FDMA_CH_USAGE_INJ;
        ch_state->phys_qu  = (u32)-1; // Currently the channel is not configured for any front port
        ch_state->prio     = inj_ch;
    }

    // For the 2.5G ports we need to use Tx flow control, i.e. checking of the
    // transmission status before actually asking the super priority port to transmit
    // the frame. We ask it to attempt to retransmit STACK_RETRANSMIT_CNT times,
    // i.e. attempt to transmit STACK_RETRANSMIT_CNT + 1 times.
    // The reason for this is that we cannot set the VTSS_FDMA_INJ_DELAY as high as we would
    // like to (it can only count to 31, but we wanted it to count to about 70 or so;
    // see GNATS #5369). For 1G ports, we are home safe even with a fully loaded shared FIFO,
    // because an VTSS_FDMA_INJ_DELAY of 24 is sufficient.
    for (phys_port = 24; phys_port <= 27; phys_port++) {
        fdma_port_state_t *port_state  = &vstate->fdma_state.fdma_port_state[phys_port];
        port_state->cfg_tx_flow_ctrl   = TRUE;
        port_state->cfg_retransmit_cnt = 5;
    }
#endif /* VTSS_OPT_FDMA_VER >= 3 */
}

/*****************************************************************************/
//
// PUBLIC FUNCTIONS
//
//****************************************************************************/

/*****************************************************************************/
// l28_fdma_init_conf_set()
/*****************************************************************************/
static vtss_rc l28_fdma_init_conf_set(vtss_state_t *const vstate)
{
    fdma_state_t *state = &vstate->fdma_state;

    if (!check_version(vstate)) {
        return VTSS_RC_ERROR; // Unsupported silicon
    }

    // Disable all channels (they must be enabled per channel through calls to
    // vtss_fdma_xtr_cfg() or vtss_fdma_inj_cfg(), or in later versions of the
    // FDMA driver through a call to vtss_fdma_cfg()).
    disable_chs(vstate, VTSS_BITMASK(VTSS_FDMA_CH_CNT));

    // Globally enable DMA controller
    VCOREII_WR(vstate, VTSS_MISC_DMA_CFG_REG, VTSS_F_DMA_EN);

    // Create new channel state defaults
    ch_state_defaults(vstate);

    state->hdr_size     = VTSS_FDMA_HDR_SIZE_BYTES;
    state->xtr_hdr_size = VTSS_FDMA_XTR_HDR_SIZE_BYTES;
    state->inj_hdr_size = VTSS_FDMA_INJ_HDR_SIZE_BYTES;
    state->xtr_ch_cnt   = 4;

    return VTSS_RC_OK;
}

/*****************************************************************************/
// l28_fdma_uninit()
/*****************************************************************************/
static vtss_rc l28_fdma_uninit(vtss_state_t *const vstate)
{
    // Globally disable DMA controller
    VCOREII_WR(vstate, VTSS_MISC_DMA_CFG_REG, 0);

    // Disable all channels
    disable_chs(vstate, VTSS_BITMASK(VTSS_FDMA_CH_CNT));

    // Disable interrupts from DMA controller
    VCOREII_WR(vstate, VTSS_INTR_MASK_TFR,   VTSS_F_INT_MASK_WE_TFR  (VTSS_BITMASK(VTSS_FDMA_CH_CNT)));
    VCOREII_WR(vstate, VTSS_INTR_MASK_BLOCK, VTSS_F_INT_MASK_WE_BLOCK(VTSS_BITMASK(VTSS_FDMA_CH_CNT)));

    // Clear pending interrupts
    clear_pending_block_done_intr(vstate, VTSS_BITMASK(VTSS_FDMA_CH_CNT));
    clear_pending_tfr_done_intr  (vstate, VTSS_BITMASK(VTSS_FDMA_CH_CNT));
    return VTSS_RC_OK;
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_xtr_cfg()
// To avoid OS-dependent allocation problems on the various OS's we support,
// we require, that the list[] is pre-allocated and that the data areas that
// its members point to are as well.
// Call this function per channel and queue.
// @ena:
//   If FALSE, the channel gets disabled and the remaining parameters (except for ch) are unused.
//   If TRUE,  the remaining parameters are used and the channel gets enabled.
/*****************************************************************************/
static vtss_rc l28_fdma_xtr_cfg(      vtss_state_t           *const vstate,
                                const BOOL                   ena,
                                const vtss_fdma_ch_t         ch,
                                const vtss_packet_rx_queue_t qu,
#if VTSS_OPT_FDMA_VER == 1
                                const unsigned int           start_gap,
                                const unsigned int           end_gap,
#endif /* VTSS_OPT_FDMA_VER == 1 */
                                      vtss_fdma_list_t       *const list,
                                      vtss_fdma_list_t       *(*xtr_cb)(void *cntxt, vtss_fdma_list_t *list, vtss_packet_rx_queue_t qu))
{
    // When using this function, channel priority is always the same as the channel number.
#if VTSS_OPT_FDMA_VER == 2
    return xtr_cfg(vstate, ena, ch, qu, 0,         0,       list, xtr_cb, ch);
#else
    return xtr_cfg(vstate, ena, ch, qu, start_gap, end_gap, list, xtr_cb, ch);
#endif /* VTSS_OPT_FDMA_VER */
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_inj_cfg()
/*****************************************************************************/
static vtss_rc l28_fdma_inj_cfg(      vtss_state_t   *const vstate,
                                const BOOL           ena,
                                const vtss_fdma_ch_t ch)
{
    // When using this function, channel priority is always the same as the channel number.
    return inj_cfg(vstate, ena, ch, ch);
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_ch_cfg()
/*****************************************************************************/
static vtss_rc l28_fdma_ch_cfg(      vtss_state_t       *const vstate,
                               const vtss_fdma_ch_t     ch,
                               const vtss_fdma_ch_cfg_t *const cfg)
{
    fdma_state_t *state   = &vstate->fdma_state;
    BOOL         ena      = TRUE;
    BOOL         call_xtr = FALSE;
    BOOL         call_inj = FALSE;

    // This is a newer edition of the older vtss_fdma_xtr_cfg() and vtss_fdma_inj_cfg()
    // functions. If the user elects to call this function, we will simply
    // pass the call onto the older functions.
    FDMA_ASSERT(NORMAL, cfg != NULL, return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, ch >= 0 && ch < VTSS_FDMA_CH_CNT, return VTSS_RC_ERROR;);

    if (cfg->usage == VTSS_FDMA_CH_USAGE_UNUSED) {
      // User wants to disable this channel. Pass it on to the correct
      // vtss_fdma_XXX_cfg() function depending on the current channel's
      // usage.
      ena = FALSE;
      if (state->fdma_ch_state[ch].usage == VTSS_FDMA_CH_USAGE_XTR) {
        call_xtr = TRUE;
      } else if (state->fdma_ch_state[ch].usage == VTSS_FDMA_CH_USAGE_INJ) {
        call_inj = TRUE;
      }
    } else if (cfg->usage == VTSS_FDMA_CH_USAGE_XTR) {
        call_xtr = TRUE;
    } else if (cfg->usage == VTSS_FDMA_CH_USAGE_INJ) {
        call_inj = TRUE;
    }

    if (call_xtr) {
#if VTSS_OPT_FDMA_VER == 1
      return xtr_cfg(vstate, ena, ch, cfg->qu, cfg->start_gap, cfg->end_gap, cfg->list, cfg->xtr_cb, cfg->prio);
#elif VTSS_OPT_FDMA_VER == 2
      return xtr_cfg(vstate, ena, ch, cfg->qu, 0,              0,            cfg->list, cfg->xtr_cb, cfg->prio);
#endif /* VTSS_OPT_FDMA_VER == 1 || 2 */
    }

    if (call_inj) {
        return inj_cfg(vstate, ena, ch, cfg->prio);
    }

    return VTSS_RC_ERROR;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_inj()
// @len: Length of frame in bytes, excluding IFH and CMD, but including FCS.
/*****************************************************************************/
static vtss_rc l28_fdma_inj(      vtss_state_t          *const vstate,
                                  vtss_fdma_list_t      *user_dcb,
                            const vtss_fdma_ch_t        inj_ch,
                            const u32                   len,
                            const vtss_fdma_inj_props_t *const props)
{
    fdma_state_t         *state = &vstate->fdma_state;
    fdma_ch_state_t      *ch_state;
    u32                  phys_port;
    fdma_sw_dcb_t        *new_tail, *sw_dcb = FDMA_SW_DCB(user_dcb);
    int                  computed_whl_frm_sz_bytes;
    u32                  ifh_offset; // To compensate for variable-position IFH
    u32                  dcb_cnt;

    FDMA_ASSERT(NORMAL, inj_ch >= 0 && inj_ch < VTSS_FDMA_CH_CNT,           return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, user_dcb,                                           return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, len >= 18 && len <= VTSS_FDMA_MAX_FRAME_SIZE_BYTES, return VTSS_RC_ERROR;); // The 18 bytes == 2 * MAC Addr + EtherType + FCS.

    ch_state  = &state->fdma_ch_state[inj_ch];
    phys_port = sw_dcb->phys_port;

    FDMA_ASSERT(NORMAL, ch_state->status != FDMA_CH_STATUS_DISABLED, return VTSS_RC_INV_STATE;);
    FDMA_ASSERT(NORMAL, ch_state->usage  == VTSS_FDMA_CH_USAGE_INJ,  return VTSS_RC_INV_STATE;);
    FDMA_ASSERT(NORMAL, phys_port < VTSS_FDMA_TOTAL_PORT_CNT,        return VTSS_RC_ERROR;);

    VTSS_NG(VTSS_TRACE_GROUP_FDMA_NORMAL, "vtss_fdma_inj(ch = %d, p = %d, l = %d).", inj_ch, phys_port, len);

    if ((ifh_offset = inj_ifh_init(vstate, sw_dcb->data, phys_port, len, props)) == 0xFFFFFFFF) {
        return VTSS_RC_INV_STATE;
    }

    new_tail = inj_dcb_init(inj_ch, sw_dcb, ifh_offset, &computed_whl_frm_sz_bytes, &dcb_cnt);
#if VTSS_OPT_FDMA_DEBUG
    FDMA_ASSERT(NORMAL, computed_whl_frm_sz_bytes == len + VTSS_FDMA_INJ_HDR_SIZE_BYTES, return VTSS_RC_ERROR;);
#endif /* VTSS_OPT_FDMA_DEBUG */

    if (new_tail == 0) {
        return VTSS_RC_ERROR;
    }

    return vtss_fdma_cmn_inj_attach_frm(vstate, sw_dcb, new_tail, inj_ch, dcb_cnt, len, props, FALSE);
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/*****************************************************************************/
// l28_fdma_tx()
/*****************************************************************************/
static vtss_rc l28_fdma_tx(vtss_state_t          *const vstate,
                           fdma_sw_dcb_t         *sw_dcb,
                           vtss_fdma_tx_info_t   *const fdma_info,
                           vtss_packet_tx_info_t *const tx_info,
                           BOOL                  afi_cancel /* unused */)
{
    vtss_fdma_ch_t inj_ch;
    fdma_sw_dcb_t  *tail;
    u32            dcb_cnt, frm_size;

    VTSS_RC(inj_ifh_init(vstate, sw_dcb, tx_info, &inj_ch, &frm_size));
    VTSS_RC(inj_dcb_init(vstate, sw_dcb, inj_ch, &tail, &dcb_cnt));
    return vtss_fdma_cmn_inj_attach_frm(vstate, sw_dcb, tail, inj_ch, dcb_cnt, frm_size /* for statistics */, fdma_info, FALSE);
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_inj_flow_ctrl_cfg_get()
// @port:           [0; VTSS_PHYS_PORT_CNT[. Conveys the front port number
//                  that the rest of the parameters pertain to.
// @tx_flow_ctrl:   Returns the current value of whether it's in TX flow
//                  control mode (TRUE) or not (FALSE).
// @retransmit_cnt: Returns the number of times a frame re-transmission will
//                  be attempted.
/*****************************************************************************/
static vtss_rc l28_fdma_inj_flow_ctrl_cfg_get(vtss_state_t *const vstate, const vtss_phys_port_no_t phys_port, BOOL *const tx_flow_ctrl, u8 *const retransmit_cnt)
{
    fdma_state_t *state = &vstate->fdma_state;
    FDMA_ASSERT(NORMAL, phys_port < VTSS_PHYS_PORT_CNT, return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, tx_flow_ctrl,                   return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, retransmit_cnt,                 return VTSS_RC_ERROR;);
    *tx_flow_ctrl   = state->fdma_port_state[phys_port].cfg_tx_flow_ctrl;
    *retransmit_cnt = state->fdma_port_state[phys_port].cfg_retransmit_cnt;
    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_inj_flow_ctrl_cfg_set()
// @port:           [0; VTSS_PHYS_PORT_CNT[. Conveys the front port number
//                  that the rest of the parameters pertain to.
// @tx_flow_ctrl:   0 or 1. If 1, software will check for super priority
//                  queue overflow prior to asking the queue system to
//                  transmit the frame. If the queue is overflown, S/W
//                  will rewind and possibly attempt a re-transmit (see
//                  next parameter).
// @retransmit_cnt: The number of times a retransmission will be attempted
//                  before dropping it. This is only used if
//                  tx_flow_ctrl parameter is 1.
/*****************************************************************************/
static vtss_rc l28_fdma_inj_flow_ctrl_cfg_set(vtss_state_t *const vstate, const vtss_phys_port_no_t phys_port, const BOOL tx_flow_ctrl, const u8 retransmit_cnt)
{
    fdma_state_t *state = &vstate->fdma_state;
    FDMA_ASSERT(NORMAL, phys_port < VTSS_PHYS_PORT_CNT, return VTSS_RC_ERROR;);
    state->fdma_port_state[phys_port].cfg_tx_flow_ctrl   = tx_flow_ctrl;
    state->fdma_port_state[phys_port].cfg_retransmit_cnt = retransmit_cnt;
    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_xtr_ch_from_list()
/*****************************************************************************/
static vtss_rc l28_fdma_xtr_ch_from_list(vtss_state_t *const vstate, const vtss_fdma_list_t *const list, vtss_fdma_ch_t *const ch)
{
    fdma_hw_dcb_t *hw_dcb;
    FDMA_ASSERT(NORMAL, list && ch, return VTSS_RC_ERROR;);
    hw_dcb = (fdma_hw_dcb_t *)list->hw_dcb;
    *ch = (hw_dcb->sar >> FDMA_F_SAR_CH_ID_FPOS) & VTSS_BITMASK(FDMA_F_SAR_CH_ID_FLEN);
    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

/*****************************************************************************/
// l28_fdma_inj_suspend_port()
// By calling this function with suspend != 0, the FDMA will not be re-started
// with *new* frames destined for the port. In order to assure that the FDMA
// is currently not injecting frames to the port, the user will have to call
// l28_fdma_inj_port_active().
// Once suspended, it cannot be resumed until possible current frames to the
// port are complete.
/*****************************************************************************/
static vtss_rc l28_fdma_inj_suspend_port(vtss_state_t *const vstate, const vtss_phys_port_no_t phys_port, const BOOL suspend)
{
    fdma_state_t *state = &vstate->fdma_state;
    FDMA_ASSERT(NORMAL, phys_port < VTSS_FDMA_TOTAL_PORT_CNT, return VTSS_RC_ERROR;);
    if (suspend) {
        // Simply set the suspended bit. This will cause a channel injecting
        // to the port to stall after the current frame has been attempted
        // injected.
        state->fdma_port_state[phys_port].suspended = TRUE;
    } else {
        /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
        FDMA_INTERRUPT_FLAGS flags;
        vtss_fdma_ch_t ch;

        FDMA_ASSERT(NORMAL, state->fdma_port_state[phys_port].suspended,       return VTSS_RC_INV_STATE;);
        FDMA_ASSERT(NORMAL, state->fdma_port_state[phys_port].active_cnt == 0, return VTSS_RC_INV_STATE;);
        // We'll have to restart all the channels whose next frame is destined
        // for the port. And we have to do these tests with interrupts (scheduler)
        // disabled. Otherwise we could end up with testing the cur_head for being
        // non-NULL, then an interrupt could remove it, then this code would check
        // the @phys_port on a now-NULL cur_head.
        FDMA_INTERRUPT_DISABLE(flags);
        // Restart channels in priority order
        for (ch = VTSS_FDMA_CH_CNT - 1; ch >= 0; ch--) {
            fdma_ch_state_t *ch_state=&state->fdma_ch_state[ch];
            if (ch_state->usage == VTSS_FDMA_CH_USAGE_INJ && ch_state->cur_head && ch_state->cur_head->phys_port == phys_port) {
                state->fdma_port_state[phys_port].active_cnt++;
                inj_ch_init(vstate, FDMA_HW_DCB(ch_state->cur_head), ch, phys_port);
                enable_ch(vstate, ch);
            }
        }
        state->fdma_port_state[phys_port].suspended = 0;
        FDMA_INTERRUPT_RESTORE(flags);
    }
    return VTSS_RC_OK;
}

/*****************************************************************************/
// l28_fdma_inj_port_active()
// Returns the number of channels that are currently attempting to inject
// to the given port.
/*****************************************************************************/
static vtss_rc l28_fdma_inj_port_active(vtss_state_t *const vstate, const vtss_phys_port_no_t phys_port, BOOL *const active)
{
    fdma_state_t *state = &vstate->fdma_state;

    FDMA_ASSERT(NORMAL, phys_port < VTSS_FDMA_TOTAL_PORT_CNT, return VTSS_RC_ERROR;);
    FDMA_ASSERT(NORMAL, active,                               return VTSS_RC_ERROR;);
    *active = state->fdma_port_state[phys_port].active_cnt > 0;

    return VTSS_RC_OK;
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_xtr_hdr_decode()
/*****************************************************************************/
static vtss_rc l28_fdma_xtr_hdr_decode(vtss_state_t *const vstate, const vtss_fdma_list_t *const list, vtss_fdma_xtr_props_t *const xtr_props)
{
    u64 ifh;
    u8  *xtr_hdr;

    FDMA_ASSERT(NORMAL, list != NULL && xtr_props != NULL && list->data != NULL && (list->act_len - (list->frm_ptr - list->ifh_ptr)) > VTSS_FDMA_XTR_HDR_SIZE_BYTES, return VTSS_RC_ERROR;);
    xtr_hdr = list->ifh_ptr;
    ifh = ((u64)xtr_hdr[0] << 32) | ((u64)xtr_hdr[1] << 40) | ((u64)xtr_hdr[2] << 48) | ((u64)xtr_hdr[3] << 56) |
          ((u64)xtr_hdr[4] <<  0) | ((u64)xtr_hdr[5] <<  8) | ((u64)xtr_hdr[6] << 16) | ((u64)xtr_hdr[7] << 24);

    memset(xtr_props, 0, sizeof(*xtr_props));

#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2
    // The stack header has been moved to the beginning of the data area if it's present. If it's not
    // present, the first byte is 0x00.
    if (list->data[0] != 0) {
        // It's present. Copy it to the xtr_props as is and set the appropriate flag.
        xtr_props->contains_stack_hdr = TRUE;
        memcpy(xtr_props->stack_hdr_bin, list->data, VTSS_VSTAX_HDR_SIZE);
    }
#endif /* defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_FDMA_VER >= 2 */

#define FDMA_IFH_GET(_ifh_, _field_) VTSS_EXTRACT_BITFIELD64((_ifh_), VTSS_FDMA_IFH_F_ ## _field_ ## _FPOS, VTSS_FDMA_IFH_F_ ## _field_ ## _FLEN)

    xtr_props->chip_no            = 0;
    xtr_props->chip_no_decoded    = TRUE;

    xtr_props->src_port           = FDMA_IFH_GET(ifh, SRC_PORT);
    xtr_props->src_port_decoded   = TRUE;

    xtr_props->was_tagged         = FDMA_IFH_GET(ifh, WAS_TAGGED);
    xtr_props->was_tagged_decoded = TRUE;

    xtr_props->tag_type           = FDMA_IFH_GET(ifh, TAG_TYPE);
    xtr_props->tag_type_decoded   = TRUE;

    xtr_props->vid                = FDMA_IFH_GET(ifh, VID);
    xtr_props->vid_decoded        = TRUE;

    xtr_props->dei                = FDMA_IFH_GET(ifh, CFI);
    xtr_props->dei_decoded        = TRUE;

    xtr_props->pcp                = FDMA_IFH_GET(ifh, UPRIO);
    xtr_props->pcp_decoded        = TRUE;

    xtr_props->frame_type         = FDMA_IFH_GET(ifh, FRM_TYPE);
    xtr_props->frame_type_decoded = TRUE;

    xtr_props->learn              = FDMA_IFH_GET(ifh, LEARN);
    xtr_props->learn_decoded      = TRUE;

    xtr_props->xtr_qu             = FDMA_IFH_GET(ifh, XTR_QU);
    xtr_props->xtr_qu_decoded     = TRUE;

    xtr_props->qos_class          = FDMA_IFH_GET(ifh, QOS);
    xtr_props->qos_class_decoded  = TRUE;

    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_fdma_stats_get()
/*****************************************************************************/
static vtss_rc l28_fdma_stats_get(vtss_state_t *const vstate, vtss_fdma_stats_t *const stats)
{
    /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    FDMA_INTERRUPT_FLAGS flags;
    fdma_state_t *state = &vstate->fdma_state;

    FDMA_ASSERT(NORMAL, stats, return VTSS_RC_ERROR;);
    FDMA_INTERRUPT_DISABLE(flags);
    memcpy(stats, &state->fdma_stats, sizeof(*stats));
    FDMA_INTERRUPT_RESTORE(flags);

    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

/*****************************************************************************/
// l28_fdma_stats_clr()
/*****************************************************************************/
static vtss_rc l28_fdma_stats_clr(vtss_state_t *const vstate)
{
    /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    FDMA_INTERRUPT_FLAGS flags;
    fdma_state_t *state = &vstate->fdma_state;

    FDMA_INTERRUPT_DISABLE(flags);
    memset(&state->fdma_stats, 0, sizeof(state->fdma_stats));
    FDMA_INTERRUPT_RESTORE(flags);

    return VTSS_RC_OK;
}

/*****************************************************************************/
// l28_fdma_irq_handler()
/*****************************************************************************/
static vtss_rc l28_fdma_irq_handler(vtss_state_t *const vstate, void *const cntxt)
{
    fdma_state_t   *state           = &vstate->fdma_state;
    u32            block_done_cause = VCOREII_RD(vstate, VTSS_INTR_STATUS_BLOCK); // The extraction channels use block done interrupts
    u32            tfr_done_cause   = VCOREII_RD(vstate, VTSS_INTR_STATUS_TFR);   // Both injection and extraction channels use transfer done interrupts.
    vtss_fdma_ch_t ch;

    state->fdma_stats.intr_cnt++;

    // The two interrupt status register must be read before continuing.
    // If the compiler decided to read the TFR_DONE after the BLOCK_DONE
    // had been processed below, we might end up not consuming all the
    // extraction buffers before attempting to restart the channel.
    VTSS_OS_REORDER_BARRIER();

    // We first handle all the block interrupts, i.e. the per-DCB extraction interrupts.
    // For the extraction interrupts, we need to clear the interrupts before iterating
    // through the DCBs. If we didn't do so, we might end up in a situation where a
    // frame arrives after we've looped through the DCBs, but before we clear the
    // interrupts, and the frame would be stuck in RAM until the next frame arrives.
    // About the same argumentation holds for injection interrupts.
    clear_pending_block_done_intr(vstate, block_done_cause);

    VTSS_OS_REORDER_BARRIER();

    // Process the interrupts from above, since that corresponds to the priority.
    for (ch = VTSS_FDMA_CH_CNT - 1; ch >= 0; ch--) {
        if (block_done_cause & VTSS_BIT(ch)) {
            if (xtr_irq_block_done(vstate, cntxt, ch)) {
                // xtr_irq_block_done() returns non-zero if the last DCB was consumed by the FDMA.
                // This should cause us to restart the channel (done in xtr_irq_tfr_done() below).
                tfr_done_cause |= (1 << ch);
            }
        }
    }

    VTSS_OS_REORDER_BARRIER();

    // The transfer done interrupts occur both when one frame has been injected and
    // in the (rare?) case where the OS cannot take off frames in the rate that the
    // FDMA extracts them to RAM, causing the DCB list to exhaust and the channel
    // to stop.
    clear_pending_tfr_done_intr(vstate, tfr_done_cause);

    VTSS_OS_REORDER_BARRIER();

    for (ch = VTSS_FDMA_CH_CNT - 1; ch >= 0; ch--) {
        if (tfr_done_cause&VTSS_BIT(ch)) {
            if (state->fdma_ch_state[ch].usage == VTSS_FDMA_CH_USAGE_INJ) {
                inj_irq_tfr_done(vstate, cntxt, ch);
            } else {
                xtr_irq_tfr_done(vstate, ch);
            }
        }
    }
    return VTSS_RC_OK;
}

#if VTSS_OPT_FDMA_VER <= 2
/*****************************************************************************/
// l28_debug_print()
/*****************************************************************************/
vtss_rc l28_debug_print(vtss_state_t *const vstate, const vtss_debug_printf_t pr, const vtss_debug_info_t *const info)
{
    fdma_state_t      *state          = &vstate->fdma_state;
    const char        *dscr_format    = "\n%-13s:";
    const char        *newline_format = "\n%-14s";
    vtss_fdma_stats_t *stats          = &state->fdma_stats;
    int               i;

    // Rx Counters:
    pr(dscr_format, "Rx Counters");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %9s%d", "Qu", i);
    }
    pr("\n");
    for (i = 0; i < 11 * VTSS_PACKET_RX_QUEUE_CNT + 14; i++) {
        pr("-");
    }
    pr(dscr_format, "Packets");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10u", stats->rx_packets[i]);
    }
    pr(dscr_format, "Bytes");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10llu", stats->rx_bytes[i]);
    }
    pr(dscr_format, "Bufs Added");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10u", stats->rx_bufs_added[i]);
    }
    pr(dscr_format, "Bufs Used");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10u", stats->rx_bufs_used[i]);
    }
    pr(dscr_format, "Bufs Left");
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10u", stats->rx_bufs_added[i] - stats->rx_bufs_used[i]);
    }

    // Read-out extraction queue drop counters.
    // This doesn't interfere with the API counter functions, since
    // the API only reads the front-port counters - not the CPU
    // port module counters.
    pr(dscr_format, "H/W Drops");
    /*lint --e{506} Constant value Boolean */
    VCOREII_WR(vstate, VTSS_DEV_C_CNTADDR(VTSS_CPU_PM_NUMBER), 0x6000);
    for (i = 0; i < VTSS_PACKET_RX_QUEUE_CNT; i++) {
        pr(" %10u", VCOREII_RD(vstate, VTSS_DEV_C_CNTDATA(VTSS_CPU_PM_NUMBER)));
    }

    pr("\n\n");

#define FDMA_CNT_PER_LINE 4
#define FDMA_PERHAPS_PRINT_NEWLINE(cnt, limit) {if ((cnt) != (limit) && (((cnt) % FDMA_CNT_PER_LINE) == (FDMA_CNT_PER_LINE - 1))) pr(newline_format, "");}

    // Tx Counters:
    pr(dscr_format, "Tx Counters");
    for (i = 0; i<VTSS_FDMA_CH_CNT; i++) {
        pr(" %9s%d", "Ch", i);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_CH_CNT - 1);
    }
    pr("\n");
    for (i = 0; i < 11*FDMA_CNT_PER_LINE + 14; i++) {
        pr("-");
    }
    pr(dscr_format, "Packets");
    for (i = 0; i < VTSS_FDMA_CH_CNT; i++) {
        pr(" %10u", stats->tx_packets[i]);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_CH_CNT - 1);
    }
    pr(dscr_format, "Bytes");
    for (i = 0; i < VTSS_FDMA_CH_CNT; i++) {
        pr(" %10llu", stats->tx_bytes[i]);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_CH_CNT - 1);
    }
    pr(dscr_format, "Bufs Used");
    for (i = 0; i < VTSS_FDMA_CH_CNT; i++) {
        pr(" %10u", stats->tx_bufs_used[i]);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_CH_CNT - 1);
    }
    pr("\n\n");

    // Drop Counters:
    pr(dscr_format, "Drop Counters");
    for (i = 0; i < VTSS_FDMA_TOTAL_PORT_CNT; i++) {
        pr(" %8s%02d", "PhysPort", i);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_TOTAL_PORT_CNT - 1);
    }
    pr("\n");
    for (i = 0; i < 11*FDMA_CNT_PER_LINE + 14; i++) {
        pr("-");
    }
    pr(dscr_format, "Rx");
    for (i = 0; i < VTSS_FDMA_TOTAL_PORT_CNT; i++) {
        pr(" %10u", i==VTSS_CPU_PM_NUMBER ? stats->rx_dropped : 0);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_TOTAL_PORT_CNT - 1);
    }
    pr(dscr_format, "Tx");
    for (i = 0; i < VTSS_FDMA_TOTAL_PORT_CNT; i++) {
        pr(" %10u", stats->tx_dropped[i]);
        FDMA_PERHAPS_PRINT_NEWLINE(i, VTSS_FDMA_TOTAL_PORT_CNT - 1);
    }
    pr("\n");

#undef FDMA_CNT_PER_LINE
#undef FDMA_PERHAPS_PRINT_NEWLINE
  return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER <= 2 */

#if VTSS_OPT_FDMA_VER >= 3
/*****************************************************************************/
// l28_debug_print()
/*****************************************************************************/
static vtss_rc l28_debug_print(vtss_state_t *const vstate, const vtss_debug_printf_t pr, const vtss_debug_info_t *const info)
{
    /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    fdma_state_t *state = &vstate->fdma_state;
    int ch, qu;
    FDMA_INTERRUPT_FLAGS flags;

    // Same statistics for chip #0 and #1. Only print once.
    pr("Interrupts: %u\n\n", state->fdma_stats.intr_cnt);
    pr("Ch Usage DCBs Used  DCBs Added Packets    Bytes          Interrupts\n");
    pr("-- ----- ---------- ---------- ---------- -------------- ----------\n");
    for (ch = 0; ch < VTSS_FDMA_CH_CNT; ch++) {
        vtss_fdma_ch_usage_t usage = state->fdma_ch_state[ch].usage;
        pr("%2d %-5s %10u %10u %10u %14llu %10u\n",
           ch,
           usage == VTSS_FDMA_CH_USAGE_UNUSED ? "Free" : usage == VTSS_FDMA_CH_USAGE_XTR ? "Xtr" : usage == VTSS_FDMA_CH_USAGE_INJ ? "Inj" : "UNKNOWN",
           state->fdma_stats.dcbs_used[ch],
           state->fdma_stats.dcbs_added[ch],
           state->fdma_stats.packets[ch],
           state->fdma_stats.bytes[ch],
           state->fdma_stats.ch_intr_cnt[ch]);
    }

    pr("\n");

    // Reading the extraction queue drop counters doesn't interfere
    // with the API counter functions, since the API only reads the
    // front-port counters - not the CPU port module counters.
    pr("Xtr Qu Packets    S/W Drops  H/W Drops\n");
    pr("------ ---------- ---------- ----------\n");
    /*lint --e{506} Constant value Boolean */
    VCOREII_WR(vstate, VTSS_DEV_C_CNTADDR(VTSS_CPU_PM_NUMBER), 0x6000);
    for (qu = 0; qu < VTSS_PACKET_RX_QUEUE_CNT; qu++) {
        pr("%6d %10u %10u %10u\n", qu, state->fdma_stats.xtr_qu_packets[qu][0], state->fdma_stats.xtr_qu_drops[qu][0], VCOREII_RD(vstate, VTSS_DEV_C_CNTDATA(VTSS_CPU_PM_NUMBER)));
    }

    pr("\n");

    return VTSS_RC_OK;
}
#endif /* VTSS_OPT_FDMA_VER >= 3 */

/*****************************************************************************/
// l28_fdma_func_init()
// Initialize current global state's function pointers.
// This is the only real public function.
/*****************************************************************************/
void l28_fdma_func_init(void)
{
    fdma_func_t *func = &vtss_state->fdma_state.fdma_func;

    // Internal functions
    func->fdma_init_conf_set         = l28_fdma_init_conf_set;
    func->fdma_xtr_qu_suspend_set    = NULL; // Not supported
    func->fdma_xtr_dcb_init          = l28_fdma_xtr_dcb_init;
    func->fdma_xtr_connect           = l28_fdma_xtr_connect;
    func->fdma_xtr_restart_ch        = l28_fdma_xtr_restart_ch;
    func->fdma_inj_restart_ch        = l28_fdma_inj_restart_ch;
#if VTSS_OPT_FDMA_VER >= 3
    func->fdma_start_ch              = l28_fdma_start_ch;
#endif /* VTSS_OPT_FDMA_VER >= 3 */

    // External functions
    func->fdma_uninit                = l28_fdma_uninit;
    func->fdma_stats_clr             = l28_fdma_stats_clr;
    func->fdma_debug_print           = l28_debug_print;
    func->fdma_irq_handler           = l28_fdma_irq_handler;
#if VTSS_OPT_FDMA_VER >= 3
    func->fdma_tx                    = l28_fdma_tx;
#endif /* VTSS_OPT_FDMA_VER >= 3 */

#if VTSS_OPT_FDMA_VER <= 2
    // Older functions
    func->fdma_inj                   = l28_fdma_inj;
    func->fdma_xtr_ch_from_list      = l28_fdma_xtr_ch_from_list;
    func->fdma_xtr_hdr_decode        = l28_fdma_xtr_hdr_decode;
    func->fdma_ch_cfg                = l28_fdma_ch_cfg;
#endif /* VTSS_OPT_FDMA_VER <= 2 */

    // L28-only functions:
#if VTSS_OPT_FDMA_VER <= 2
    func->fdma_xtr_cfg               = l28_fdma_xtr_cfg;
    func->fdma_stats_get             = l28_fdma_stats_get;
    func->fdma_inj_flow_ctrl_cfg_get = l28_fdma_inj_flow_ctrl_cfg_get;
    func->fdma_inj_flow_ctrl_cfg_set = l28_fdma_inj_flow_ctrl_cfg_set;
    func->fdma_inj_cfg               = l28_fdma_inj_cfg;
#endif /* VTSS_OPT_FDMA_VER */
    func->fdma_inj_suspend_port      = l28_fdma_inj_suspend_port;
    func->fdma_inj_port_active       = l28_fdma_inj_port_active;
}

#endif /* VTSS_ARCH_LUTON28 && VTSS_OPT_FDMA */

/*****************************************************************************/
//
// End of file
//
//****************************************************************************/
