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

#ifndef _VTSS_MAIN_H_
#define _VTSS_MAIN_H_

#include <stdlib.h>
#include <ctype.h>
#include <cyg/kernel/kapi.h>
#include <sys/types.h>

#include "main_types.h"
#include "vtss_api.h"
#include "control_api.h"

/* - Semaphores ----------------------------------------------------- */
typedef cyg_sem_t vtss_os_sem_t;
#define VTSS_OS_SEM_CREATE(sem, count) cyg_semaphore_init(sem, count)
#define VTSS_OS_SEM_WAIT(sem)          cyg_semaphore_wait(sem)
#define VTSS_OS_SEM_POST(sem)          cyg_semaphore_post(sem)

/* - Critical region protection using a mutex ----------------------- */
typedef cyg_mutex_t vtss_os_crit_t;
#define VTSS_OS_CRIT_CREATE(crit)      cyg_mutex_init(crit)
#define VTSS_OS_CRIT_ENTER(crit)       cyg_mutex_lock(crit)
#define VTSS_OS_CRIT_EXIT(crit)        cyg_mutex_unlock(crit)

/* - Sleeping ------------------------------------------------------- */
#define ECOS_MSECS_PER_HWTICK   (CYGNUM_HAL_RTC_NUMERATOR / CYGNUM_HAL_RTC_DENOMINATOR / 1000000)
#define ECOS_TICKS_PER_SEC      (1000 / (ECOS_MSECS_PER_HWTICK))
#define ECOS_HWCLOCKS_PER_MSEC  (CYGNUM_HAL_RTC_PERIOD / ECOS_MSECS_PER_HWTICK)

// Avoid "Warning -- Constant value Boolean" Lint error
#define VTSS_OS_MSEC2TICK(msec)  /*lint -save -e506 */ MAX(1, (msec) / ECOS_MSECS_PER_HWTICK) /*lint -restore */ // returns at least a single tick
#define VTSS_OS_TICK2MSEC(tick)  ((tick) * ECOS_MSECS_PER_HWTICK)
#define VTSS_OS_MSLEEP(msec)     cyg_thread_delay(VTSS_OS_MSEC2TICK(msec))

/* - Assertions ----------------------------------------------------- */
/* Allow for assertion-is-going-to-fail callback. */
typedef void (*vtss_common_assert_cb_t)(const char *file_name, const unsigned long line_num, const char *msg);
extern vtss_common_assert_cb_t vtss_common_assert_cb;
void vtss_common_assert_cb_set(vtss_common_assert_cb_t cb);

/* Note, that the vtss_common_assert_cb() function will be called whether or
   not the image is compiled assertions enabled. */
#define VTSS_COMMON_ASSERT_CB() {if(vtss_common_assert_cb) vtss_common_assert_cb(__FILE__, __LINE__, "Assertion failed");}
/* Trying hard to avoid expr to be evaluated twice (which may cause side effects if it's a function call for instance, or a register read with read-side-effects). */
#define VTSS_ASSERT(expr) {if(!(expr)) {VTSS_COMMON_ASSERT_CB(); control_system_assert_do_reset(); }}

/* - Error codes ---------------------------------------------------- */
#define VTSS_OK                VTSS_RC_OK
#define VTSS_UNSPECIFIED_ERROR VTSS_RC_ERROR
#define VTSS_INVALID_PARAMETER VTSS_RC_ERROR
#define VTSS_INCOMPLETE        VTSS_RC_INCOMPLETE  /* Operation incomplete */
#define vtss_error_txt(rc) (rc == VTSS_OK ? "VTSS_OK" : "VTSS_RC_ERROR")

#define VTSS_RC(expr) { vtss_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define ARRSZ(t) /*lint -e{574} */ (sizeof(t)/sizeof(t[0])) /* Suppress Lint Warning 574: Signed-unsigned mix with relational */

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

/* Macros for stringification */
#define vtss_xstr(s) vtss_str(s)
#define vtss_str(s) #s

/* Thread default values */
// To avoid starvation of the Network Alarm thread we must assign it
// a THREAD_ABOVE_NORMAL_PRIO priority, or better, we must define our
// thread priorities based on the Network Alarm priority, but the
// constant for doing this (CYGPKG_NET_FAST_THREAD_PRIORITY) is only
// available if <network.h> is included. Including <network.h> causes
// a bunch of compile-time errors for the modules that require _KERNEL
// to be defined. Therefore, the priorities are hardcoded here:

#define THREAD_DEFAULT_STACK_SIZE  8192
#define THREAD_BELOW_NORMAL_PRIO   8
#define THREAD_DEFAULT_PRIO        7
#define THREAD_ABOVE_NORMAL_PRIO   6
#define THREAD_HIGH_PRIO           5
#define THREAD_HIGHEST_PRIO        4

/* Lock/unlock functions for get-modify-set API operations */
void vtss_appl_api_lock(void);
void vtss_appl_api_unlock(void);

/* ================================================================= *
 *  Bit field macros           
 * ================================================================= */

/* Bit field macros */
#define VTSS_BF_SIZE(n)      (((n)+7)/8)
#define VTSS_BF_GET(a, n)    ((a[(n)/8] & (1<<((n)%8))) ? 1 : 0)
#define VTSS_BF_SET(a, n, v) { if (v) { a[(n)/8] |= (1U<<((n)%8)); } else { a[(n)/8] &= ~(1U<<((n)%8)); }}
#define VTSS_BF_CLR(a, n)    (memset(a, 0, VTSS_BF_SIZE(n)))

/* Port member bit field macros */
#define VTSS_PORT_BF_SIZE                VTSS_BF_SIZE(VTSS_PORTS)
#define VTSS_PORT_BF_GET(a, port_no)     VTSS_BF_GET(a, port_no - VTSS_PORT_NO_START)
#define VTSS_PORT_BF_SET(a, port_no, v)  VTSS_BF_SET(a, port_no - VTSS_PORT_NO_START, v)
#define VTSS_PORT_BF_CLR(a)              VTSS_BF_CLR(a, VTSS_PORTS)

#ifndef VTSS_BITOPS_DEFINED
#ifdef __ASSEMBLER__
#define VTSS_BIT(x)                   (1 << (x))
#define VTSS_BITMASK(x)               ((1 << (x)) - 1)
#else
#define VTSS_BIT(x)                   (1U << (x))
#define VTSS_BITMASK(x)               ((1U << (x)) - 1)
#endif
#define VTSS_EXTRACT_BITFIELD(x,o,w)  (((x) >> (o)) & VTSS_BITMASK(w))
#define VTSS_ENCODE_BITFIELD(x,o,w)   (((x) & VTSS_BITMASK(w)) << (o))
#define VTSS_ENCODE_BITMASK(o,w)      (VTSS_BITMASK(w) << (o))
#define VTSS_BITOPS_DEFINED
#endif /* VTSS_BITOPS_DEFINED */

/* ================================================================= *
 *  Error codes                
 * ================================================================= */

/* First module specific error code. Can be used as first entry in error type enumeration */
#define MODULE_ERROR_START(module_id) (-((module_id<<16) | 0xffff))
/* Decompose return code into module_id and error code */
#define VTSS_RC_GET_MODULE_ID(rc)   (((-rc) >> 16) & 0xffff)
#define VTSS_RC_GET_MODULE_CODE(rc) (-(rc & 0xffff))

/* Error code interpretation */
const char *error_txt(vtss_rc);

/* ================================================================= *
 *  Initialization             
 * ================================================================= */

/* Flags for INIT_CMD_CONF_DEF */
#define INIT_CMD_PARM2_FLAGS_IP      0x00000001 /* If set, restore IP configuration */ 
#define INIT_CMD_PARM2_FLAGS_ME_PRIO 0x00000002 /* If set, restore Master Election Priority (topo) */
#define INIT_CMD_PARM2_FLAGS_SID     0x00000004 /* If set, restore USID mappings (topo) */

/* Initialize all modules */
vtss_rc init_modules(vtss_init_data_t *data);

/* ================================================================= *
 *  Other useful, yet fundamental, constants and types
 * ================================================================= */
#define VTSS_MAC_ADDR_SZ_BYTES    6

/* MAC address, simpler to use than vtss_mac_t */
typedef uchar mac_addr_t[6];

void vtss_api_trace_update(void);

BOOL vtss_stacking_enabled(void);

#ifndef VTSS_SW_OPTION_PHY
#define PHY_INST NULL
#endif

#endif /* _VTSS_MAIN_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
