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

*/

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"

#include "cli.h"
#include "msg_api.h"
#include "mirror.h"
#include "mirror_api.h" // For mirror_conf_t
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */


/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/

// See mirror_icli_functions.h
void mirror_destination(i32 session_id, BOOL monitor, BOOL destination, BOOL interface, icli_port_type_t port_type, icli_switch_port_range_t port_id, BOOL no)
{
    mirror_conf_t        conf;
    mirror_mgmt_conf_get(&conf);
    vtss_rc  rc;

    vtss_port_no_t uport, iport;

    uport = port_id.begin_uport;
    iport = uport2iport(uport);

    T_I("Session_Id:%u, monitor:%d, destination:%d, interface:%d, port_type:%d, iport:%u, uport:%u, no:%d",
        session_id, monitor, destination, interface, port_type, iport, uport, no);

    if (iport && PORT_NO_IS_STACK(iport)) {
        ICLI_PRINTF("Stack port %lu cannot be destination port\n", uport);
        return;
    }

    if (no) { // No command
        conf.dst_port = MIRROR_PORT_DEFAULT; // Disabling mirroring
        conf.mirror_switch = MIRROR_SWITCH_DEFAULT;
    } else {
        conf.dst_port = iport;
        conf.mirror_switch = port_type;
    }

    if ((rc = mirror_mgmt_conf_set(&conf)) != VTSS_OK) {
        ICLI_PRINTF("\n%s\n", error_txt(rc));
    }
}


// See mirror_icli_functions.h
void mirror_source(i32 session_id, BOOL monitor, BOOL destination, BOOL interface, icli_port_type_t port_type,
                   icli_stack_port_range_t *port_list, BOOL cpu, icli_range_t *cpu_switch_range, BOOL both, BOOL rx, BOOL tx, BOOL no)
{
    switch_iter_t   sit;
    mirror_switch_conf_t switch_conf;
    vtss_rc rc;

    // If no parameters then default to both.
    if (!both && !rx && !tx) {
        both = TRUE;
    }

    T_I("Interface:%d, cpu:%d, no:%d", interface, cpu, no);

    (void) icli_switch_iter_init(&sit);
    while (icli_switch_iter_getnext(&sit, port_list)) {
        port_iter_t pit;
        // Get current configuration
        if ((rc = mirror_mgmt_switch_conf_get(sit.isid, &switch_conf)) != VTSS_OK) {
            ICLI_PRINTF("\n%s\n", error_txt(rc));
            continue;
        }

        // Loop through all ports
        if (interface) {
            if (icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK)) {
                while (icli_port_iter_getnext(&pit, port_list)) {

                    T_I("session_id:%u, port:%u, usid:%d, no:%d, both:%d, rx:%d, tx:%d", session_id, pit.uport, sit.usid, no, both, rx, tx);

                    if (no) {// No command
                        switch_conf.src_enable[pit.iport] = FALSE;
                        switch_conf.dst_enable[pit.iport] = FALSE;
                    } else {
                        switch_conf.src_enable[pit.iport] = both | rx;
                        switch_conf.dst_enable[pit.iport] = both | tx;
                    }
                }
            }
        }

#ifdef VTSS_FEATURE_MIRROR_CPU
        if (cpu) {
            T_I("session_id:%u, no:%d, both:%d, rx:%d, tx:%d, switch_conf.cpu_src_enable:%d, switch_conf.cpu_dst_enable:%d", session_id, no, both, rx, tx, switch_conf.cpu_src_enable, switch_conf.cpu_dst_enable );
            if (no) {
                switch_conf.cpu_src_enable = FALSE;
                switch_conf.cpu_dst_enable = FALSE;
            } else {
                switch_conf.cpu_src_enable = both | rx;
                switch_conf.cpu_dst_enable = both | tx;
            }

            T_I("session_id:%u, no:%d, both:%d, rx:%d, tx:%d, switch_conf.cpu_src_enable:%d, switch_conf.cpu_dst_enable:%d", session_id, no, both, rx, tx, switch_conf.cpu_src_enable, switch_conf.cpu_dst_enable );

        }
#endif //VTSS_FEATURE_MIRROR_CPU
        mirror_mgmt_switch_conf_set(sit.isid, &switch_conf);
    }
}


// Runtime function for ICLI that determine if CPU mirroring is supported (e.g. Jaguar doesn't support this)
BOOL mirror_cpu_runtime(u32                session_id,
                        icli_runtime_ask_t ask,
                        icli_runtime_t     *runtime)
{


    switch (ask) {
    case ICLI_ASK_PRESENT:
#ifdef VTSS_FEATURE_MIRROR_CPU
        return TRUE;
#endif
    default :
        return FALSE;
    }
}



// Runtime function for ICLI that determine if CPU switch id is needed (for stacking)
BOOL mirror_switch_id_runtime(u32                session_id,
                              icli_runtime_ask_t ask,
                              icli_runtime_t     *runtime)
{
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
    switch (ask) {
    case ICLI_ASK_PRESENT:
        return TRUE;
    case ICLI_ASK_RANGE:
        runtime->range.type = ICLI_RANGE_TYPE_SIGNED;
        runtime->range.u.sr.cnt  = 1;
        runtime->range.u.sr.range[0].min  = VTSS_ISID_START;
        runtime->range.u.sr.range[0].max  = VTSS_ISID_END - VTSS_ISID_START;
        return TRUE;

    default :
        return FALSE;
    }

#else
    return FALSE;
#endif

}

#ifdef VTSS_SW_OPTION_ICFG

/* ICFG callback functions */

// ICFG for the global configuration
static vtss_rc mirror_global_conf(const vtss_icfg_query_request_t *req,
                                  vtss_icfg_query_result_t *result)
{
    mirror_conf_t        conf;
    mirror_mgmt_conf_get(&conf);
    char str_buf[100];
    vtss_port_no_t iport;
    vtss_isid_t isid;
    mirror_switch_conf_t switch_conf;


    // Print out if not default value (or if requested to print all)
    // Mirror switch and destination port
    if (req->all_defaults ||
        (conf.dst_port != MIRROR_PORT_DEFAULT) ||
        (conf.mirror_switch != MIRROR_SWITCH_DEFAULT)) {
        (void) icli_port_info_txt(topo_isid2usid(conf.mirror_switch), iport2uport(conf.dst_port), str_buf);
        T_I("conf.mirror_switch:%d, conf.dst_port:%u, buf:%s", conf.mirror_switch, conf.dst_port, str_buf);
        VTSS_RC(vtss_icfg_printf(result, "monitor destination interface %s\n",
                                 str_buf));
    }


    // Loop through all switches in a stack
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (msg_switch_exists(isid)) {
            if (mirror_mgmt_switch_conf_get(isid, &switch_conf) != VTSS_RC_OK) {
                T_E("Could not get mirror cpu conf");
            }

#ifdef VTSS_FEATURE_MIRROR_CPU
            // Print out if not default value (or if requested to print all)
            if (req->all_defaults ||
                (switch_conf.cpu_src_enable != MIRROR_CPU_SRC_ENA_DEFAULT) ||
                (switch_conf.cpu_dst_enable != MIRROR_CPU_DST_ENA_DEFAULT)) {

                VTSS_RC(vtss_icfg_printf(result, "monitor source cpu "));


#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
                // Print CPU switch ID in case that it is supported (for stacking)
                ICLI_PRINTF(" %d", topo_isid2usid(isid));
#endif

                VTSS_RC(vtss_icfg_printf(result, " %s\n",
                         switch_conf.cpu_src_enable ? switch_conf.cpu_dst_enable ? "Both" : "Rx" :
                                         switch_conf.cpu_dst_enable ? "Tx" : "Disabled"));
            }
#endif

            for (iport = 0; iport < VTSS_PORTS; iport++) {
                // Print out if not default value (or if requested to print all)
                if (req->all_defaults ||
                    (switch_conf.src_enable[iport] != MIRROR_SRC_ENA_DEFAULT) ||
                    (switch_conf.dst_enable[iport] != MIRROR_DST_ENA_DEFAULT)) {

                    // Get the interface as text
                    (void) icli_port_info_txt(topo_isid2usid(isid), iport2uport(iport), str_buf);


                    VTSS_RC(vtss_icfg_printf(result, "monitor source interface %s %s\n",
                                             str_buf,
                         switch_conf.src_enable[iport] ? switch_conf.dst_enable[iport] ? "Both" : "Rx" :
                                             switch_conf.dst_enable[iport]  ? "Tx" : "Disabled"));
                }

            }
        }
    }
    return VTSS_RC_OK;
}




/* ICFG Initialization function */
vtss_rc mirror_lib_icfg_init(void)
{
    vtss_rc rc;

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MIRROR_GLOBAL_CONF, mirror_global_conf)) != VTSS_OK) {
        return rc;
    }

    return rc;
}
#endif // VTSS_SW_OPTION_ICFG

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
