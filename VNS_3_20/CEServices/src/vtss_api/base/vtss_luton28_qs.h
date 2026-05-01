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

#ifndef _VTSS_LUTON28_QS_H_
#define _VTSS_LUTON28_QS_H_

/* Luton28 queue system settings */

/* VTSS_<maxsize>_<prios><strict/weighted priority>_<drop/flowcontrol mode>_<register> */
/* define   ::= VTSS_(NORM|9600)_(1|2|4)(S|W)_(DROP|FC)_<register> */
/* register ::= EARLY_TX | FWDP_(START|STOP) | IMIN | IDISC | EMIN | EMAX(0-3) | ZEROPAUSE | PAUSEVALUE */

/* level:    Memory usage in slices */
/* wmaction: 0 = Drop from head, 1 = Drop during reception, 2 = Urgent, 3 = FC */
/* cmpwith:  0 = Total, 1 = Ingress, 2 = Spec, 3 = Egress */
/* qmask:    Which queues to apply, 1=q0, 2=q1, 4=q2, 8=q3, 16=CPU  */
/* chkmin:   Check IMIN WM */
/* lowonly:  Apply only to lowest prio queue with data. */

/* Luton28 strict priority, drop mode */
#define VTSS_NORM_4S_DROP_LEVEL_0    19
#define VTSS_NORM_4S_POLICER_LEVEL_0 12   /* Used if policer enabled */
#define VTSS_NORM_4S_DROP_ACTION_0   2
#define VTSS_NORM_4S_DROP_CMPWITH_0  0
#define VTSS_NORM_4S_DROP_QMASK_0    0x1F
#define VTSS_NORM_4S_DROP_CHKMIN_0   1
#define VTSS_NORM_4S_DROP_LOWONLY_0  0

#define VTSS_NORM_4S_DROP_LEVEL_1    0
#define VTSS_NORM_4S_DROP_ACTION_1   2
#define VTSS_NORM_4S_DROP_CMPWITH_1  2
#define VTSS_NORM_4S_DROP_QMASK_1    0x01
#define VTSS_NORM_4S_DROP_CHKMIN_1   0
#define VTSS_NORM_4S_DROP_LOWONLY_1  0

#define VTSS_NORM_4S_DROP_LEVEL_2    0
#define VTSS_NORM_4S_DROP_ACTION_2   2
#define VTSS_NORM_4S_DROP_CMPWITH_2  2
#define VTSS_NORM_4S_DROP_QMASK_2    0x03
#define VTSS_NORM_4S_DROP_CHKMIN_2   0
#define VTSS_NORM_4S_DROP_LOWONLY_2  0

#define VTSS_NORM_4S_DROP_LEVEL_3    0
#define VTSS_NORM_4S_DROP_ACTION_3   2
#define VTSS_NORM_4S_DROP_CMPWITH_3  2
#define VTSS_NORM_4S_DROP_QMASK_3    0x07
#define VTSS_NORM_4S_DROP_CHKMIN_3   0
#define VTSS_NORM_4S_DROP_LOWONLY_3  0

#define VTSS_NORM_4S_DROP_LEVEL_4    1
#define VTSS_NORM_4S_DROP_ACTION_4   2
#define VTSS_NORM_4S_DROP_CMPWITH_4  2
#define VTSS_NORM_4S_DROP_QMASK_4    0x7
#define VTSS_NORM_4S_DROP_CHKMIN_4   0
#define VTSS_NORM_4S_DROP_LOWONLY_4  0

#define VTSS_NORM_4S_DROP_LEVEL_5    21
#define VTSS_NORM_4S_POLICER_LEVEL_5 14   /* Used if policer enabled */
#define VTSS_NORM_4S_DROP_ACTION_5   1
#define VTSS_NORM_4S_DROP_CMPWITH_5  1
#define VTSS_NORM_4S_DROP_QMASK_5    0xf
#define VTSS_NORM_4S_DROP_CHKMIN_5   1
#define VTSS_NORM_4S_DROP_LOWONLY_5  0

#define VTSS_NORM_4S_DROP_IMIN          0
#define VTSS_NORM_4S_DROP_EARLY_TX      0
#define VTSS_NORM_4S_DROP_EMIN          3
#define VTSS_NORM_4S_DROP_EMAX0         14
#define VTSS_NORM_4S_DROP_EMAX1         14
#define VTSS_NORM_4S_DROP_EMAX2         14
#define VTSS_NORM_4S_DROP_EMAX3         14
#define VTSS_NORM_4S_DROP_ZEROPAUSE     0
#define VTSS_NORM_4S_DROP_PAUSEVALUE    0 /*Don't care*/


/* Luton28 weighted priority, drop mode */
#define VTSS_NORM_4W_DROP_LEVEL_0    17
#define VTSS_NORM_4W_POLICER_LEVEL_0 12   /* Used if policer enabled */
#define VTSS_NORM_4W_DROP_ACTION_0   2
#define VTSS_NORM_4W_DROP_CMPWITH_0  0
#define VTSS_NORM_4W_DROP_QMASK_0    0x1F
#define VTSS_NORM_4W_DROP_CHKMIN_0   1
#define VTSS_NORM_4W_DROP_LOWONLY_0  0

#define VTSS_NORM_4W_DROP_LEVEL_1    31
#define VTSS_NORM_4W_DROP_ACTION_1   0
#define VTSS_NORM_4W_DROP_CMPWITH_1  0
#define VTSS_NORM_4W_DROP_QMASK_1    0
#define VTSS_NORM_4W_DROP_CHKMIN_1   0
#define VTSS_NORM_4W_DROP_LOWONLY_1  0

#define VTSS_NORM_4W_DROP_LEVEL_2    31
#define VTSS_NORM_4W_DROP_ACTION_2   0
#define VTSS_NORM_4W_DROP_CMPWITH_2  0
#define VTSS_NORM_4W_DROP_QMASK_2    0
#define VTSS_NORM_4W_DROP_CHKMIN_2   0
#define VTSS_NORM_4W_DROP_LOWONLY_2  0

#define VTSS_NORM_4W_DROP_LEVEL_3    31
#define VTSS_NORM_4W_DROP_ACTION_3   0
#define VTSS_NORM_4W_DROP_CMPWITH_3  0
#define VTSS_NORM_4W_DROP_QMASK_3    0
#define VTSS_NORM_4W_DROP_CHKMIN_3   0
#define VTSS_NORM_4W_DROP_LOWONLY_3  0

#define VTSS_NORM_4W_DROP_LEVEL_4    31
#define VTSS_NORM_4W_DROP_ACTION_4   0
#define VTSS_NORM_4W_DROP_CMPWITH_4  0
#define VTSS_NORM_4W_DROP_QMASK_4    0
#define VTSS_NORM_4W_DROP_CHKMIN_4   0
#define VTSS_NORM_4W_DROP_LOWONLY_4  0

#define VTSS_NORM_4W_DROP_LEVEL_5    31
#define VTSS_NORM_4W_POLICER_LEVEL_5 14   /* Used if policer enabled */
#define VTSS_NORM_4W_DROP_ACTION_5   1
#define VTSS_NORM_4W_DROP_CMPWITH_5  1
#define VTSS_NORM_4W_DROP_QMASK_5    0xf
#define VTSS_NORM_4W_DROP_CHKMIN_5   1
#define VTSS_NORM_4W_DROP_LOWONLY_5  0

#define VTSS_NORM_4W_DROP_IMIN          0
#define VTSS_NORM_4W_DROP_EARLY_TX      0
#define VTSS_NORM_4W_DROP_EMIN          3
#define VTSS_NORM_4W_DROP_EMAX0         14
#define VTSS_NORM_4W_DROP_EMAX1         14
#define VTSS_NORM_4W_DROP_EMAX2         14
#define VTSS_NORM_4W_DROP_EMAX3         14
#define VTSS_NORM_4W_DROP_ZEROPAUSE     0
#define VTSS_NORM_4W_DROP_PAUSEVALUE    0 /*Don't care*/

/* Luton28 flow control mode */
/* Note! */
/* 2.5GB Stack ports need modified WM: */
/* #define VTSS_NORM_4S_FC_LEVEL_2    4 */


/* Luton28 flow control mode */
#define VTSS_NORM_4S_FC_LEVEL_0    31
#define VTSS_NORM_4S_FC_ACTION_0   0
#define VTSS_NORM_4S_FC_CMPWITH_0  1
#define VTSS_NORM_4S_FC_QMASK_0    0x1F
#define VTSS_NORM_4S_FC_CHKMIN_0   1
#define VTSS_NORM_4S_FC_LOWONLY_0  1

#define VTSS_NORM_4S_FC_LEVEL_1    15
#define VTSS_NORM_4S_FC_ACTION_1   2
#define VTSS_NORM_4S_FC_CMPWITH_1  1
#define VTSS_NORM_4S_FC_QMASK_1    0x1F
#define VTSS_NORM_4S_FC_CHKMIN_1   1
#define VTSS_NORM_4S_FC_LOWONLY_1  1

#define VTSS_NORM_4S_FC_LEVEL_2    6
#define VTSS_NORM_4S_FC_ACTION_2   3
#define VTSS_NORM_4S_FC_CMPWITH_2  1
#define VTSS_NORM_4S_FC_QMASK_2    0x1F
#define VTSS_NORM_4S_FC_CHKMIN_2   0
#define VTSS_NORM_4S_FC_LOWONLY_2  0		   

#define VTSS_NORM_4S_FC_LEVEL_3    4
#define VTSS_NORM_4S_FC_ACTION_3   3
#define VTSS_NORM_4S_FC_CMPWITH_3  1
#define VTSS_NORM_4S_FC_QMASK_3    0x1F
#define VTSS_NORM_4S_FC_CHKMIN_3   0
#define VTSS_NORM_4S_FC_LOWONLY_3  0		   

#define VTSS_NORM_4S_FC_LEVEL_4    31
#define VTSS_NORM_4S_FC_ACTION_4   0
#define VTSS_NORM_4S_FC_CMPWITH_4  0
#define VTSS_NORM_4S_FC_QMASK_4    0
#define VTSS_NORM_4S_FC_CHKMIN_4   0
#define VTSS_NORM_4S_FC_LOWONLY_4  0		

#define VTSS_NORM_4S_FC_LEVEL_5    31
#define VTSS_NORM_4S_FC_ACTION_5   0
#define VTSS_NORM_4S_FC_CMPWITH_5  0
#define VTSS_NORM_4S_FC_QMASK_5    0
#define VTSS_NORM_4S_FC_CHKMIN_5   0
#define VTSS_NORM_4S_FC_LOWONLY_5  0		

#define VTSS_NORM_4S_FC_IMIN          0
#define VTSS_NORM_4S_FC_EARLY_TX      0		   
#define VTSS_NORM_4S_FC_EMIN          3
#define VTSS_NORM_4S_FC_EMAX0         6
#define VTSS_NORM_4S_FC_EMAX1         6
#define VTSS_NORM_4S_FC_EMAX2         6
#define VTSS_NORM_4S_FC_EMAX3         6
#define VTSS_NORM_4S_FC_ZEROPAUSE     1
#define VTSS_NORM_4S_FC_PAUSEVALUE    0xFF

/* Special settings for 10 Mbps half duplex operation */
#define VTSS_NORM_10_HDX_EMIN         0
#define VTSS_NORM_10_HDX_EMAX0        1
#define VTSS_NORM_10_HDX_EMAX1        1
#define VTSS_NORM_10_HDX_EMAX2        1
#define VTSS_NORM_10_HDX_EMAX3        1

#endif /* _VTSS_LUTON28_QS_H_ */
