/*

 Vitesse Switch API software.

 Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
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
/*
 This is the central place to disable management of the various QoS features
 in CLI, WEB and XML for Jaguar and Luton26.

 This is implemented by using a set of defines that can be examined in the code,
 and makes it possible to release the various features in a controlled manner
 according to the release plan.

 The features in WEB management can also be controlled in menu_default.txt.

 Some of the features depends also on the build type (SMB or CE), but this
 is directly specified in the management code by using VTSS_SW_OPTION_BUILD_SMB
 and VTSS_SW_OPTION_BUILD_CE. This makes it possible to remove parts of the
 code by using a script.

*/

#ifndef _VTSS_QOS_FEATURES_H_
#define _VTSS_QOS_FEATURES_H_

#ifdef VTSS_ARCH_JAGUAR_1
//#define QOS_FEATURE_DISABLE_QUEUE_POLICER
#endif

#ifdef VTSS_ARCH_LUTON26
//#define QOS_FEATURE_DISABLE_QUEUE_POLICER
#endif

#ifdef VTSS_ARCH_SERVAL
//#define QOS_FEATURE_DISABLE_QUEUE_POLICER
#endif

#endif /* _VTSS_QOS_FEATURES_H_ */
