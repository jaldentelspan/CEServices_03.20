/*

 Vitesse Switch API software.

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

#ifndef _LUTON28_REG_H_
#define _LUTON28_REG_H_

/* Naming conventions:
   B_<blk>      : Block <blk>
   S_<sub>      : Sub block <sub>
   R_<blk>_<reg>: Block <blk>, register <reg>
*/

/* ================================================================= *
 * Block IDs
 * ================================================================= */
#define B_PORT     1  /* Port (Sub Blocks: 0-15) */
#define B_ANALYZER 2  /* Analyzer (Sub Block: 0) */
#define B_MIIM     3  /* MII Management (Sub Blocks: 0 or 0-1 depending on chip) */
#define B_MEMINIT  3  /* Memory Initialization (Sub Block: 1 or 2 depending on chip) */
#define B_ACL      3  /* ACL */
#define B_VAUI     3  /* VAUI */
#define B_CAPTURE  4  /* CPU Capture Buffer (Sub Blocks: 0-3,4,6,7) depending on chip */
#define B_ARBITER  5  /* Arbiter (Sub Block: 0) */
#define B_PORT_HI  6  /* Port 16-31 (Sub Blocks: 0-15) */
#define B_SYSTEM   7  /* System Registers (Sub Block: 0) */


#define S_MEMINIT        2  /* Memory initialization sub block */
#define S_ACL            3
#define S_VAUI           4
#define S_CAPTURE_DATA   0 /* CPU Capture Buffer Data */
#define S_CAPTURE_STATUS 4 /* CPU Capture Buffer Status Sub Block: 4 */
#define S_CAPTURE_RESET  7 /* CPU Capture Buffer Reset Sub Block: 7 */

/* ================================================================= *
 * B_PORT::PORTREG registers
 * ================================================================= */
#define R_PORT_REGMODE 0x01 /* Device Register Mode */

/* ================================================================= *
 * B_PORT::MAC registers
 * ================================================================= */
#define R_PORT_MAC_CFG     0x00 /* MAC Config */
#define R_PORT_MACHDXGAP   0x02 /* Half Duplex Gaps */
#define R_PORT_FCCONF      0x04 /* Flow Control transmit */
#define R_PORT_FCMACHI     0x08 /* Flow Control SMAC High */
#define R_PORT_FCMACLO     0x0C /* Flow Control SMAC Low */
#define R_PORT_MAXLEN      0x10 /* Max Length */
#define R_PORT_ADVPORTM    0x19 /* Advanced Port Mode Setup */
#define R_PORT_TXUPDCFG    0x24 /* Transmit Modify Setup */
#define R_PORT_PCSCTRL     0x18 /* PCS Control */
#define R_PORT_SGMII_CFG   0x1A /* SGMII Configuration */
#define R_PORT_PCSSTAT     0x1C /* PCS Status */
#define R_PORT_SGMIIOBS    0x1D /* SGMII Observe */
#define R_PORT_STACKCFG    0x25 /* Stack configuration */
#define R_PORT_SHAPER_CFG  0x28 /* Shaper Setup */
#define R_PORT_POLICER_CFG 0x29 /* Policer Setup */

/* ================================================================= *
 * B_PORT::Shared FIFO registers
 * ================================================================= */
#define R_PORT_CPUTXDAT    0xC0 /* CPU Transmit DATA */
#define R_PORT_MISCFIFO    0xC4 /* Misc Control Register */
#define R_PORT_MISCSTAT    0xC8 /* Misc Status */
#define R_PORT_MISCCFG     0xC5 /* Miscellaneous Configuration */
#define R_PORT_CPUTXHDR    0xCC /* CPU Transmit HEADER */
#define R_PORT_FREEPOOL    0xD8 /* Free RAM Counter */
#define R_PORT_Q_MISC_CONF 0xDF /* Misc Pool Control */
#define R_PORT_Q_EGRESS_WM 0xE0 /* Egress Max Watermarks */
#define R_PORT_Q_INGRESS_WM 0xE8 /* 0xE8-0xED: Ingress Watermarks */

/* ================================================================= *
 * B_PORT::Categorizer registers
 * ================================================================= */

#define R_PORT_CAT_PR_DSCP_VAL_0_3 0x61 /* Categorizer DSCP Values 0-3 */
#define R_PORT_CAT_DROP            0x6E /* Categorizer Frame Dropping */
#define R_PORT_CAT_PR_MISC_L2      0x6F /* Categorizer Misc L2 QoS */
#define R_PORT_CAT_VLAN_MISC       0x79 /* Categorizer VLAN Misc */
#define R_PORT_CAT_PORT_VLAN       0x7A /* Categorizer Port VLAN */
#define R_PORT_CAT_OTHER_CFG       0x7B /* Categorizer Other Configuration */
#define R_PORT_CAT_GEN_PRIO_REMAP  0x7D /* Categorizer Generic Priority Remap */
#define R_PORT_CAT_QCE0            0x80 /* 0x80-0x8B: QCE 0-11 */
#define R_PORT_CAT_QCL_RANGE_CFG   0x8C /* QCL Range Configuration */
#define R_PORT_CAT_QCL_DEFAULT_CFG 0x8D /* QCL Default Configuration */

/* ================================================================= *
 * B_PORT::Counter registers
 * ================================================================= */
#define R_PORT_C_CNTDATA    0xDC /* Counter Data */
#define R_PORT_C_CNTADDR    0xDD /* Counter Address */
#define R_PORT_C_CLEAR      0x52 /* Clear Counters */

/* ================================================================= *
 * B_ANALYZER registers
 * ================================================================= */
#define R_ANALYZER_ADVLEARN     0x03 /* Advanced Learning Setup */
#define R_ANALYZER_IFLODMSK     0x04 /* IP Multicast Flood Mask */
#define R_ANALYZER_VLANMASK     0x05 /* VLAN Source Port Mask */
#define R_ANALYZER_MACHDATA     0x06 /* Mac Address High */
#define R_ANALYZER_MACLDATA     0x07 /* Mac Address Low */
#define R_ANALYZER_ANMOVED      0x08 /* Station Move Logger */
#define R_ANALYZER_ANAGEFIL     0x09 /* Aging Filter */
#define R_ANALYZER_ANEVENTS     0x0A /* Event Sticky Bits */
#define R_ANALYZER_ANCNTMSK     0x0B /* Event Sticky Mask */
#define R_ANALYZER_ANCNTVAL     0x0C /* Event Sticky Counter */
#define R_ANALYZER_LERNMASK     0x0D /* Learn Mask */
#define R_ANALYZER_UFLODMSK     0x0E /* Unicast Flood Mask */
#define R_ANALYZER_MFLODMSK     0x0F /* Multicast Flood Mask */
#define R_ANALYZER_RECVMASK     0x10 /* Receive Mask */
#define R_ANALYZER_AGGRCNTL     0x20 /* Aggregation Mode */
#define R_ANALYZER_AGGRMSKS     0x30 /* 0x30-0x3F Aggregation Masks */
#define R_ANALYZER_DSTMASKS     0x40 /* 0x40-0x7F Destination Port Masks */
#define R_ANALYZER_SRCMASKS     0x80 /* Source Port Masks */
#define R_ANALYZER_CAPENAB      0xA0 /* Capture Enable */
#define R_ANALYZER_CAPQUEUE     0xA1 /* Capture Queue */
#define R_ANALYZER_LEARNDROP    0xA2 /* Learning Drop */
#define R_ANALYZER_LEARNAUTO    0xA3 /* Learning Auto */
#define R_ANALYZER_LEARNCPU     0xA4 /* Learning CPU */
#define R_ANALYZER_STORMLIMIT   0xAA /* Storm Control */
#define R_ANALYZER_STORMLIMIT_ENA 0xAB  /* Storm Control Enable */
#define R_ANALYZER_CAPQUEUEGARP 0xA8 /* Capture Queue GARP */
#define R_ANALYZER_ACLPOLIDX    0xA9 /* ACL Policer Index */
#define R_ANALYZER_PVLAN_MASK   0xAC /* Private VLAN Mask */
#define R_ANALYZER_I6FLODMSK    0xAD /* IPv6 Multicast Flood Mask */
#define R_ANALYZER_STORMPOLUNIT 0xAE /* Storm Policer Unit */
#define R_ANALYZER_AUTOAGE      0xB1 /* Auto Age Timer */
#define R_ANALYZER_STACKPORTSA  0xF1 /* Stack Ports A */
#define R_ANALYZER_STACKPORTSB  0xF2 /* Stack Ports B */
#define R_ANALYZER_STACKCFG     0xF3 /* Stack Configuration */
#define R_ANALYZER_MIRRORPORTS  0xF4 /* Mirror Ports */
#define R_ANALYZER_DSCPMODELOW  0xF8 /* DSCP Mode Low Ports */
#define R_ANALYZER_DSCPMODEHIGH 0xF9 /* DSCP Mode High Ports */
#define R_ANALYZER_DSCPSELLOW   0xFA /* DSCP Enable Low Ports */
#define R_ANALYZER_DSCPSELHIGH  0xFB /* DSCP Enable High Ports */
#define R_ANALYZER_EMIRRORMASK  0xFC /* Egress Mirror Mask */
#define R_ANALYZER_MACACCES     0xB0 /* Mac Table Command */
#define R_ANALYZER_MACTINDX     0xC0 /* Mac Table Index */
#define R_ANALYZER_VLANACES     0xD0 /* VLAN Table Command */
#define R_ANALYZER_VLANTINDX    0xE0 /* VLAN Table Index */
#define R_ANALYZER_AGENCNTL     0xF0 /* Analyzer Config Register */

/* Commands for Mac Table Command register */
#define MAC_CMD_IDLE        0  /* Idle */
#define MAC_CMD_LEARN       1  /* Insert (Learn) 1 entry */
#define MAC_CMD_FORGET      2  /* Delete (Forget) 1 entry */
#define MAC_CMD_TABLE_AGE   3  /* Age entire table */
#define MAC_CMD_GET_NEXT    4  /* Get next entry */
#define MAC_CMD_TABLE_CLEAR 5  /* Delete all entries in table */
#define MAC_CMD_READ        6  /* Read entry at Mac Table Index */
#define MAC_CMD_WRITE       7  /* Write entry at Mac Table Index */

/* Commands for VLAN Table Command register */
#define VLAN_CMD_IDLE        0  /* Idle */
#define VLAN_CMD_READ        1  /* Read entry at VLAN Table Index */
#define VLAN_CMD_WRITE       2  /* Write entry at VLAN Table Index */
#define VLAN_CMD_TABLE_CLEAR 3  /* Reset all entries in table */

/* Modes for Aggregation Mode register */
#define AGGRCNTL_MODE_SMAC         1   /* SMAC only */
#define AGGRCNTL_MODE_DMAC         2   /* DMAC only */
#define AGGRCNTL_MODE_SMAC_DMAC    3   /* SMAC xor DMAC */
#define AGGRCNTL_MODE_SMAC_DMAC_IP 0   /* SMAC xor DMAC xor IP-info */
#define AGGRCNTL_MODE_PSEUDORANDOM 4   /* Pseudo randomized */
#define AGGRCNTL_MODE_IP           5   /* IP-info only */

/* ================================================================= *
 * B_MIIM registers
 * ================================================================= */
#define R_MIIM_MIIMSTAT 0x00 /* MII-M Status */
#define R_MIIM_MIIMCMD  0x01 /* MII-M Command */
#define R_MIIM_MIIMDATA 0x02 /* MII-M Return data */
#define R_MIIM_MIIMPRES 0x03 /* MII-M Prescaler */
#define R_MIIM_MIIMSCAN 0x04 /* MII-M Scan setup */
#define R_MIIM_MIIMSRES 0x05 /* MII-M Scan Results */

/* ================================================================= *
 * B_MEMINIT registers
 * ================================================================= */
#define R_MEMINIT_MEMINIT 0x00 /* Memory Initialization */
#define R_MEMINIT_MEMRES  0x01 /* Memory Initialization Result */

/* Commands for Memory Initialization register */
#define MEMINIT_CMD_INIT  0x10101 /* Initialize Memory */
#define MEMINIT_CMD_READ  0x00200 /* Read Result */

/* Results for Memory Initialization Result register */
#define MEMRES_OK         0x3  /* Result OK */

/* Maximum Memory ID and skipped memory IDs */
#define MEMINIT_MAXID   36
#define MEMID_SKIP_MIN  7
#define MEMID_SKIP_MAX  8

/* ================================================================= *
 * B_ACL registers
 * ================================================================= */
#define R_ACL_ACL_CFG             0x00 /* ACL General Configuration */
#define R_ACL_PAG_CFG             0x01 /* Port to PAG mapping */

#define R_ACL_TCP_RNG_ENA_CFG_0   0x1D /* 0x1D-0x2B: UDP/TCP range */
#define R_ACL_TCP_RNG_VALUE_CFG_0 0x1E

#define ACE_DATA_OFFS             0x2D /* 0x2D-0x64: ACE data */
#define ACE_MASK_OFFS             0x65 /* 0x65-0x9C: ACE mask */

/* ACE Ingress and Egress Actions */
#define R_ACL_IN_VLD              0x9D
#define R_ACL_IN_MISC_CFG         0x9E
#define R_ACL_IN_REDIR_CFG        0x9F
#define R_ACL_IN_CNT              0xA0
#define R_ACL_EG_VLD              0xA1
#define R_ACL_EG_PORT_MASK        0xA2
#define R_ACL_EG_CNT              0xA3

/* ACE Control */
#define R_ACL_UPDATE_CTRL         0xA5
#define R_ACL_MV_CFG              0xA6
#define R_ACL_STATUS              0xA7
#define R_ACL_STICKY              0xA8

/* MAC_ETYPE ACE */
#define R_ACL_ETYPE_TYPE          0x00
#define R_ACL_ETYPE_L2_SMAC_HIGH  0x01
#define R_ACL_ETYPE_L2_SMAC_LOW   0x02
#define R_ACL_ETYPE_L2_DMAC_HIGH  0x03
#define R_ACL_ETYPE_L2_DMAC_LOW   0x04
#define R_ACL_ETYPE_L2_ETYPE      0x05

/* MAC_LLC ACE */
#define R_ACL_LLC_TYPE            0x08
#define R_ACL_LLC_L2_SMAC_HIGH    0x09
#define R_ACL_LLC_L2_SMAC_LOW     0x0A
#define R_ACL_LLC_L2_DMAC_HIGH    0x0B
#define R_ACL_LLC_L2_DMAC_LOW     0x0C
#define R_ACL_LLC_L2_LLC          0x0E

/* MAC_SNAP ACE */
#define R_ACL_SNAP_TYPE           0x10
#define R_ACL_SNAP_L2_SMAC_HIGH   0x11
#define R_ACL_SNAP_L2_SMAC_LOW    0x12
#define R_ACL_SNAP_L2_DMAC_HIGH   0x13
#define R_ACL_SNAP_L2_DMAC_LOW    0x14
#define R_ACL_SNAP_L2_SNAP_LOW    0x15
#define R_ACL_SNAP_L2_SNAP_HIGH   0x16

/* ARP ACE */
#define R_ACL_ARP_TYPE            0x18
#define R_ACL_ARP_L2_SMAC_HIGH    0x19
#define R_ACL_ARP_L2_SMAC_LOW     0x1A
#define R_ACL_ARP_L3_ARP          0x1B
#define R_ACL_ARP_L3_IPV4_DIP     0x1C
#define R_ACL_ARP_L3_IPV4_SIP     0x1D

/* IPV4_TCP_UDP ACE */
#define R_ACL_UDP_TCP_TYPE        0x20
#define R_ACL_UDP_TCP_L3_MISC_CFG 0x21
#define R_ACL_UDP_TCP_L3_IPV4_DIP 0x22
#define R_ACL_UDP_TCP_L3_IPV4_SIP 0x23
#define R_ACL_UDP_TCP_L4_PORT     0x24
#define R_ACL_UDP_TCP_L4_MISC     0x25

/* IPV4_OTHER ACE */
#define R_ACL_IPV4_TYPE           0x28
#define R_ACL_IPV4_L3_MISC_CFG    0x29
#define R_ACL_IPV4_L3_IPV4_DIP    0x2A
#define R_ACL_IPV4_L3_IPV4_SIP    0x2B
#define R_ACL_IPV4_DATA_0         0x2C
#define R_ACL_IPV4_DATA_1         0x2D

/* IPV6 ACE */
#define R_ACL_IPV6_TYPE           0x30
#define R_ACL_IPV6_L3_MISC_CFG    0x31
#define R_ACL_IPV6_L3_IPV6_SIP_0  0x32
#define R_ACL_IPV6_L3_IPV6_SIP_1  0x33
#define R_ACL_IPV6_L3_IPV6_SIP_2  0x34
#define R_ACL_IPV6_L3_IPV6_SIP_3  0x35

/* ACL update commands */
#define ACL_CMD_WRITE             0
#define ACL_CMD_READ              1
#define ACL_CMD_MOVE_UP           2
#define ACL_CMD_MOVE_DOWN         3
#define ACL_CMD_INIT              4
#define ACL_CMD_RESET_CNT         5

/* ACL types */
#define ACL_TYPE_ETYPE            0
#define ACL_TYPE_LLC              1
#define ACL_TYPE_SNAP             2
#define ACL_TYPE_ARP              3
#define ACL_TYPE_UDP_TCP          4
#define ACL_TYPE_IPV4             5
#define ACL_TYPE_IPV6             6

/* ACL PAG width */
#define ACL_PAG_WIDTH 8

/* ================================================================= *
 * B_VAUI registers
 * ================================================================= */

#define R_VAUI_LCPLL_CFG             0x00
#define R_VAUI_LCPLL_TEST_CFG        0x01
#define R_VAUI_LCPLL_STATUS          0x02
#define R_VAUI_RCOMP_CFG             0x03
#define R_VAUI_LANE_CFG              0x04
#define R_VAUI_LANE_TEST_CFG         0x05
#define R_VAUI_IB_CFG                0x06
#define R_VAUI_OB_CFG                0x07
#define R_VAUI_LANE_STATUS           0x14
#define R_VAUI_IB_STATUS             0x15
#define R_VAUI_LANE_TESTPATTERN_CFG  0x1C
#define R_VAUI_SIGDET_CFG            0x1D
#define R_VAUI_SIGDET_TEST           0x1E
#define R_VAUI_ANEG_CFG              0x1F
#define R_VAUI_ANEG_TEST             0x20
#define R_VAUI_ANEG_ADV_ABILITY_0    0x21
#define R_VAUI_ANEG_ADV_ABILITY_1    0x22
#define R_VAUI_ANEG_NEXT_PAGE_0      0x23
#define R_VAUI_ANEG_NEXT_PAGE_1      0x24
#define R_VAUI_ANEG_LP_ADV_ABILITY_0 0x37
#define R_VAUI_ANEG_LP_ADV_ABILITY_1 0x38
#define R_VAUI_ANEG_STATUS           0x39
#define R_VAUI_ANEG_DEBUG            0x3A

/* ================================================================= *
 * B_CAPTURE registers
 * ================================================================= */
#define R_CAPTURE_FRAME_DATA 0x00 /* Frame Data (Sub Block depends on chip and CPU queue) */

/* Internal Frame Header, IFH0 (bit 63-32) and IFH1 (bit 31-0) */

#define O_IFH_VSTAX     63     /* 63: IFH VStaX header present flag */
#define M_IFH_VSTAX     0x1
#define O_IFH_LENGTH    48     /* 61-48: Length */
#define M_IFH_LENGTH    0x3fff
#define O_IFH_PORT      42     /* 46-42: Port */
#define M_IFH_PORT      0x1f
#define O_IFH_UPRIO     29     /* 31-29: Tag priority */
#define M_IFH_UPRIO     0x7
#define O_IFH_CFI       28     /* 28   : CFI */
#define M_IFH_CFI       0x1
#define O_IFH_VID       16     /* 27-16: VID */
#define M_IFH_VID       0xfff
#define O_IFH_TAG       16     /* 31-16: VID, CFI, UPRIO */
#define M_IFH_TAG       0xffff
#define O_IFH_STACK_B   14     /* 14   : Send to stack link B */
#define M_IFH_STACK_B   0x1
#define O_IFH_STACK_A   13     /* 13   : Send to stack link A */
#define M_IFH_STACK_A   0x1
#define O_IFH_AGGR_CODE 10     /* 13-10: Aggregation Code. */
#define M_IFH_AGGR_CODE 0xf
#define O_IFH_TAG_TYPE  9      /* 9    : Tag type */
#define M_IFH_TAG_TYPE  0x1
#define O_IFH_FRM_TYPE  6      /* 8-6  : Frame type */
#define M_IFH_FRM_TYPE  0x7
#define O_IFH_TAGGED    5      /* 5    : Tagged */
#define M_IFH_TAGGED    0x1
#define O_IFH_LEARN     4      /* 4    : Learn */
#define M_IFH_LEARN     0x1
#define O_IFH_XTR_QU    2      /* 3-2  : Extraction queue */
#define M_IFH_XTR_QU    0x3
#define O_IFH_COS       0      /* 1-0  : QoS class */
#define M_IFH_COS       0x3

/* Get field from IFH0/IFH1 */
#define IFH_GET(ifh0, ifh1, field) ((O_IFH_##field > 31 ? (ifh0 >> (O_IFH_##field - 32)) : (ifh1 >> O_IFH_##field)) & M_IFH_##field)

/* Put field to IFH0/IFH1 */
#define IFH_PUT(ifh0, ifh1, field, val) { ifh0 |= (O_IFH_##field > 31 ? ((val)<<(O_IFH_##field - 32)) : 0); ifh1 |= (O_IFH_##field > 31 ? 0 : (val)<<O_IFH_##field); }

#define EPID_VSTAX      0x8000 /* Vitesse Ethernet Protocol ID for VStaX */

/* VStaX header format */
#define O_VSH_ETYPE     80      /* 95-80: Ethernet Type */
#define M_VSH_ETYPE     0xffff
#define O_VSH_EPID      64      /* 79-64: Ethernet Protocol ID */
#define M_VSH_EPID      0xffff
#define O_VSH_FORMAT    63      /* 63: Format (0) */
#define M_VSH_FORMAT    0x1
#define O_VSH_DP        61      /* 66-61: Drop Precedence */
#define M_VSH_DP        0x3
#define O_VSH_SPRIO     59      /* 59: Super Priority */
#define M_VSH_SPRIO     0x1
#define O_VSH_IPRIO     56      /* 58-56: Internal priority */
#define M_VSH_IPRIO     0x7
#define O_VSH_IDM       55      /* 55: Ingress Drop Mode */
#define M_VSH_IDM       0x1
#define O_VSH_DMAC_UNKN 53      /* 53: DMAC Unknown */
#define M_VSH_DMAC_UNKN 0x1
#define O_VSH_TTL       48      /* 52-48: TTL */
#define M_VSH_TTL       0x1f
#define O_VSH_LRN_SKIP  47      /* 47: Learning Skip */
#define M_VSH_LRN_SKIP  0x1
#define O_VSH_FWD_MODE  44      /* 46-44: Forward Mode */
#define M_VSH_FWD_MODE  0x7
#define O_VSH_DST_UPSID 37      /* 41-37: Destination Unit Port Set ID */
#define M_VSH_DST_UPSID 0x1f
#define O_VSH_DST_PORT  32      /* 36-32: Destination Port/Queue */
#define M_VSH_DST_PORT  0x1f
#define O_VSH_UPRIO     29      /* 31-29: User Priority */
#define M_VSH_UPRIO     0x7
#define O_VSH_CFI       28      /* 28: CFI */
#define M_VSH_CFI       0x1
#define O_VSH_VID       16      /* 27-16: VID */
#define M_VSH_VID       0xfff
#define O_VSH_TAGGED    15      /* 15: Tagged flag */
#define M_VSH_TAGGED    0x1
#define O_VSH_STAG      14      /* 14: S-Tag Type */
#define M_VSH_STAG      0x1
#define O_VSH_ISOLATED  13      /* 13: Isolated Port */
#define M_VSH_ISOLATED  0x1
#define O_VSH_SRC_MODE  10      /* 10: Source address mode */
#define M_VSH_SRC_MODE  0x1
#define O_VSH_SRC_UPSID 5       /* 9-5: Source Unit Port Set ID */
#define M_VSH_SRC_UPSID 0x1f
#define O_VSH_SRC_PORT  0       /* 4-0: Source Port */
#define M_VSH_SRC_PORT  0x1f

/* Get field from VSH1/VSH2 */
#define VSH_GET(vsh1, vsh2, field) ((O_VSH_##field > 31 ? (vsh1 >> (O_VSH_##field - 32)) : (vsh2 >> O_VSH_##field)) & M_VSH_##field)

/* Put field to VSH1/VSH2 */
#define VSH_PUT(vsh1, vsh2, field, val) { vsh1 |= (O_VSH_##field > 31 ? ((val)<<(O_VSH_##field - 32)) : 0); vsh2 |= (O_VSH_##field > 31 ? 0 : (val)<<O_VSH_##field); }

/* ================================================================= *
 * B_ARBITER registers
 * ================================================================= */
#define R_ARBITER_ARBEMPTY     0x0C /* Arbiter Empty */
#define R_ARBITER_ARBDISC      0x0E /* Arbiter Discard */
#define R_ARBITER_ARBBURSTPROB 0x15 /* Burst Probability */
#define R_ARBITER_ARBHOLBPENAB 0x17 /* Head of Line Blocking Protection */

/* ================================================================= *
 * B_SYSTEM registers
 * ================================================================= */
#define R_SYSTEM_CPUMODE       0x00 /* CPU Transfer Mode */
#define R_SYSTEM_SIPAD         0x01 /* SI Padding */
#define R_SYSTEM_PICONF        0x02 /* PI Config */
#define R_SYSTEM_MACRO_CTRL    0x08 /* Hardware semaphore */
#define R_SYSTEM_HWSEM         0x13 /* Hardware semaphore */
#define R_SYSTEM_GLORESET      0x14 /* Global Reset */
#define R_SYSTEM_ICPU_MBOX_VAL 0x15 /* MBOX value */
#define R_SYSTEM_ICPU_MBOX_SET 0x16 /* MBOX set */
#define R_SYSTEM_ICPU_MBOX_CLR 0x17 /* MBOX clear */
#define R_SYSTEM_CHIPID        0x18 /* Chip Identification */
#define R_SYSTEM_TIMECMP       0x24 /* Time Compare Value */
#define R_SYSTEM_SLOWDATA      0x2C /* SlowData */
#define R_SYSTEM_CPUCTRL       0x30 /* CPU/interrupt Control */
#define R_SYSTEM_CAPCTRL       0x31 /* Capture Control */
#define R_SYSTEM_GPIOCTRL      0x33 /* GPIO Control */
#define R_SYSTEM_GPIO          0x34 /* General Purpose IO */
#define R_SYSTEM_SIMASTER      0x35 /* SI Master Interface */
#define R_SYSTEM_RATEUNIT      0x36 /* Policer/shaper rate unit */
#define R_SYSTEM_LEDTIMER      0x3C /* LED timer */
#define R_SYSTEM_LEDMODES      0x3D /* LED modes */
#define R_SYSTEM_SGMII_TR_DBG  0x08 /* SGMII debug */
#define R_SYSTEM_ICPU_CTRL     0x10 /* Internal CPU Control */
#define R_SYSTEM_ICPU_ADDR     0x11 /* Internal CPU On-Chip RAM Address */
#define R_SYSTEM_ICPU_DATA     0x12 /* Internal CPU On-Chip RAM Data */

#define VTSS_T_RESET      125000  /* Waiting time (nanoseconds) after reset command */

#endif /* _LUTON28_REG_H */

