
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
#ifdef VTSS_SW_OPTION_EEE
#include "mgmt_api.h"
#include "eee.h"
#undef VTSS_TRACE_MODULE_ID // We use the port module trace in this file
#undef TRACE_GRP_CRIT // We use the port module trace in this file
#include "eee_api.h"
#endif // #ifdef VTSS_SW_OPTION_EEE

#include "port.h" // For trace

#include "msg_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif



#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */


#ifdef VTSS_SW_OPTION_EEE
/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/


static char *yn_txt(BOOL val)
{
  return val ? "Yes" : "No";
}

// Function for printing EEE mode and urgent queues configuration
// In : session_id - Need for ICLI_PRINTF
//      usid - User switch id
//      uport - User port number
//      iport - Internal port number
//      eee_switch_conf - EEE configuration.
//      eee_switch_state - EEE status state
static void port_power_savings_eee_show(i32 session_id, vtss_isid_t usid, vtss_port_no_t uport, vtss_port_no_t iport,
                                        eee_switch_conf_t *eee_switch_conf, eee_switch_state_t *eee_switch_state, BOOL print_header,
                                        BOOL eee, BOOL actiphy, BOOL perfectreach)
{
  port_status_t      port_status;

  if (port_mgmt_status_get(topo_usid2isid(usid), iport, &port_status) != VTSS_RC_OK) {
    T_E("Could not get port status");
    return;
  }


  port_info_t port_info;

  (void)port_info_get(iport, &port_info);
  if (!port_info.phy) {
    return;
  }

  // Print out table header
  if (print_header) {
    ICLI_PRINTF("Port Lnk");


    if (actiphy) {
      ICLI_PRINTF(" Energy-detect");
    }

    if (perfectreach) {
      ICLI_PRINTF(" Short-Reach");
    }

    if (eee) {
      ICLI_PRINTF(" EEE Cap LPCap PowerSave\n");
    }

    ICLI_PRINTF("\n");
  }


  ICLI_PRINTF("%-4lu %-3s",
              uport,
              yn_txt(eee_switch_state->port[iport].link));

  if (actiphy) {
    ICLI_PRINTF(" %-13s", port_status.power.actiphy_capable ? yn_txt(port_status.power.actiphy_power_savings) : "N/A");
  }

  if (perfectreach) {
    ICLI_PRINTF(" %-11s", port_status.power.perfectreach_capable ? yn_txt(port_status.power.perfectreach_power_savings) : "N/A");
  }

  if (eee) {
    //    ICLI_PRINTF("%-12s %-3s %-3s %-5s %-9s",
    ICLI_PRINTF(" %-7s %-5s %-5s",
                eee_switch_state->port[iport].eee_capable ? yn_txt(eee_switch_conf->port[iport].eee_ena) : "N/A",
                eee_switch_state->port[iport].eee_capable ? yn_txt(eee_switch_state->port[iport].link_partner_eee_capable) : "N/A",
                eee_switch_state->port[iport].eee_capable ? yn_txt(eee_switch_state->port[iport].rx_in_power_save_state || eee_switch_state->port[iport].tx_in_power_save_state) : "N/A");
  }
  ICLI_PRINTF("\n");
}


// Function for setting EEE mode configuration
// In : iport - Internal port number
//      eee_switch_conf - EEE configuration.
//      eee_switch_state - EEE status state
//      no  - TRUE to disable EEE
//      interface - TRUE if user has specified a specific interface port
static void port_power_savings_eee_mode_conf(vtss_port_no_t iport, eee_switch_conf_t *eee_switch_conf,
                                             eee_switch_state_t *eee_switch_state, BOOL no, BOOL interface)
{
  if (no) {// No command
    eee_switch_conf->port[iport].eee_ena = FALSE;
  } else {
    // Enable eee if the port is eee capable or if the user has specified a specific interface
    T_I_PORT(iport, "eee_capable:%d, interface:%d", eee_switch_state->port[iport].eee_capable, interface);
    if (eee_switch_state->port[iport].eee_capable || interface) {
      eee_switch_conf->port[iport].eee_ena = TRUE;
    }
  }
}


// Function for setting EEE urgent queues configuration
// In : iport - Internal port number
//      eee_switch_conf - EEE configuration.
//      eee_switch_state - EEE status state
//      no  - TRUE to disable EEE
//      interface - TRUE if user has specified a specific interface port
//      urgent_uq_list - List with queues that are urgent

#if EEE_FAST_QUEUES_CNT > 0
// See port_power_savings_icli_functions.h
static void port_power_savings_eee_urgent_queues(i32 session_id, vtss_port_no_t iport, eee_switch_conf_t *eee_switch_conf,
                                                 eee_switch_state_t *eee_switch_state, BOOL no, BOOL interface, icli_range_t *urgent_qu_list)
{
  u8 qu_index;
  u8 range_index;

  // Urgent_Qu_List can be NULL if the uses simply want to disable all urgent queues
  if (urgent_qu_list != NULL) {
    T_I("interface:%d,u.sr.cnt:%u", interface, urgent_qu_list->u.sr.cnt);

    // Loop through the urgent queues list
    for (qu_index = 0; qu_index < urgent_qu_list->u.sr.cnt; qu_index++) {
      T_I_PORT(iport,  "min:%d, max:%d, no%d", urgent_qu_list->u.sr.range[qu_index].min, urgent_qu_list->u.sr.range[qu_index].max, no);
      for (range_index = urgent_qu_list->u.sr.range[qu_index].min; range_index <= urgent_qu_list->u.sr.range[qu_index].max; range_index++) {

        // Make sure we don't get out of bounce
        if ((range_index < EEE_FAST_QUEUES_MIN) || (range_index > EEE_FAST_QUEUES_MAX)) {
          ICLI_PRINTF("%% Ignoring invalid queue:%d. Valid range is %lu-%lu\n", range_index, EEE_FAST_QUEUES_MIN, EEE_FAST_QUEUES_MAX);
          continue;
        }

        if (no) {// No command
          eee_switch_conf->port[iport].eee_fast_queues &= ~(u8)(1 << (range_index - EEE_FAST_QUEUES_MIN));
        } else {

          T_I_PORT(iport, "qu:%d, cap:%d, if:%d, range:%d",
                   eee_switch_conf->port[iport].eee_fast_queues, eee_switch_state->port[iport].eee_capable, interface, range_index);

          // Enable eee if the port is eee capable or if the user has specified a specific interface
          if (eee_switch_state->port[iport].eee_capable || interface) {
            eee_switch_conf->port[iport].eee_fast_queues |= (1 << (range_index - EEE_FAST_QUEUES_MIN));
          }
          T_I_PORT(iport, "qu:%d", eee_switch_conf->port[iport].eee_fast_queues);
        }
      }
    }
  } else {
    if (no) {
      eee_switch_conf->port[iport].eee_fast_queues = 0;
    }
  }
}
#endif // EEE_FAST_QUEUES_CNT
#endif // #ifdef VTSS_SW_OPTION_EEE


#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
static void port_power_savings_power(vtss_port_no_t iport, vtss_isid_t isid,  BOOL actiphy, BOOL no)
{
  port_conf_t        port_conf;
  port_status_t      port_status;

  // Get port configuration
  if (port_mgmt_conf_get(isid, iport, &port_conf) != VTSS_OK) {
    T_E("Could not get port configuration");
    return;
  }

  // Getting greenEthernet status
  if (port_mgmt_status_get(isid, iport, &port_status) != VTSS_RC_OK) {
    T_E("Could not get port status for port:%u", iport);
    return;
  }


  if (actiphy) {
    if (!port_status.power.actiphy_capable) {
      T_E("Port:%u doesn't support energy-detect", iport);
      return;
    }

    if (no) {
      port_conf.power_mode &= ~VTSS_PHY_POWER_ACTIPHY; // Clear the actiphy bit.
    } else {
      port_conf.power_mode |= VTSS_PHY_POWER_ACTIPHY; //  Set the actiphy bit.
    }
  } else {
    // Not Actiphy mean PerfectReach
    if (!port_status.power.perfectreach_capable) {
      T_E("Port:%u doesn't support short-reach", iport);
      return;
    }

    if (no) {
      port_conf.power_mode &= ~VTSS_PHY_POWER_DYNAMIC; // Clear the PerfectReach bit.
    } else {
      port_conf.power_mode |= VTSS_PHY_POWER_DYNAMIC; //  Set the PerfectReach bit.
    }

  }


  //
  // Do the configuration update
  //
  if (port_mgmt_conf_set(isid, iport, &port_conf) != VTSS_OK) {
    T_E_PORT(iport, "Could not set port configuration");
  }
}
#endif /* VTSS_SW_OPTION_PHY_POWER_CONTROL */





// Common function used for looping though all switches and ports in a stack
// In : session_id - Need for ICLI_PRINTF
//      show - TRUE if it is the ICLI show command that is calling the function// In : session_id - Need for ICLI_PRINTF
//      mode - TRUE if it is the ICLI enable/disable command that is calling the function// In : session_id - Need for ICLI_PRINTF
//      urgent_queues - TRUE if it is the ICLI urgent queues command that is calling the function// In : session_id - Need for ICLI_PRINTF
//      interface - TRUE if is a command is called with a specific interface
//      actiphy - TRUE is it actiphy that shall be set.
//      eee_switch_conf - EEE configuration.
//      eee_switch_state - EEE status state
//      no  - TRUE to disable EEE
void port_power_savings_common(i32 session_id, BOOL mode, BOOL urgent_queues, BOOL has_interface, BOOL actiphy, BOOL perfectreach,
                               icli_stack_port_range_t *port_list, icli_range_t *urgent_queues_list, BOOL no, BOOL show_eee,
                               BOOL show_actiphy, BOOL show_perfect_reach)
{
#ifdef VTSS_SW_OPTION_EEE
  eee_switch_conf_t eee_switch_conf;
  eee_switch_state_t eee_switch_state;
  BOOL print_header = TRUE;
  vtss_rc rc;
#endif
  switch_iter_t   sit;



  // Loop through all switches in stack
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, port_list)) {
      port_iter_t pit;


#ifdef VTSS_SW_OPTION_EEE
      // Get configuration for the current switch
      if ((rc = eee_mgmt_switch_conf_get(sit.isid, &eee_switch_conf)) != VTSS_RC_OK) {
        CPRINTF("Conf get: %s\n", error_txt(rc));
        return;
      }

      // Get state for the current switch
      if ((rc = eee_mgmt_switch_state_get(sit.isid, &eee_switch_state)) != VTSS_RC_OK) {
        CPRINTF("State get: %s\n", error_txt(rc));
        return;
      }
#endif // VTSS_SW_OPTION_EEE
      // Loop through all ports
      if (icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK) == VTSS_RC_OK) {
        while (icli_port_iter_getnext(&pit, port_list)) {

          // Call the function for the ICLI caller
#ifdef VTSS_SW_OPTION_EEE
          T_D_PORT(pit.iport, "session_id:%u, usid:%d, no:%d, interface:%d, mode:%d, urgent_queues:%d",
                   session_id, sit.usid, no, has_interface, mode, urgent_queues);

          if (show_eee || show_actiphy || show_perfect_reach) {
            port_power_savings_eee_show(session_id, sit.usid, pit.uport, pit.iport, &eee_switch_conf, &eee_switch_state, print_header, show_eee, show_actiphy, show_perfect_reach);
            print_header = FALSE; // Only print header once
          }

          if (mode) {
            port_power_savings_eee_mode_conf(pit.iport, &eee_switch_conf, &eee_switch_state, no, has_interface);
          }

#if EEE_FAST_QUEUES_CNT > 0
          if (urgent_queues) {
            port_power_savings_eee_urgent_queues(session_id, pit.iport, &eee_switch_conf, &eee_switch_state, no, has_interface, urgent_queues_list);
          }
#endif

#endif // VTSS_SW_OPTION_EEE
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
          if (actiphy) {
            port_power_savings_power(pit.iport, sit.isid, TRUE, no);
          }

          if (perfectreach) {
            port_power_savings_power(pit.iport, sit.isid, FALSE, no);
          }
#endif //#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
        }
      }
    }

#ifdef VTSS_SW_OPTION_EEE
    // If it is a set command the update the configuration
    if (mode || urgent_queues) {
      if (eee_mgmt_switch_conf_set(sit.isid, &eee_switch_conf) != VTSS_RC_OK) {
        T_E("Could not set EEE conf");
      }
    }
#endif // VTSS_SW_OPTION_EEE
  }
}

#ifdef VTSS_SW_OPTION_EEE

// Function for setting the optimize for power or latency.
// In : optimized_for_power - TRUE to optimized for power else optimize for latency
void port_power_savings_eee_optimize_for_power(BOOL optimize_for_power)
{
  eee_switch_global_conf_t switch_global_conf;
  // Get current configuration
  if (eee_mgmt_switch_global_conf_get(&switch_global_conf) != VTSS_RC_OK) {
    T_E("Could not get EEE configuration");
  }

  // Do the change
  switch_global_conf.optimized_for_power = optimize_for_power;

  // Write the new configuration
  if (eee_mgmt_switch_global_conf_set(&switch_global_conf) != VTSS_RC_OK) {
    T_E("Could not set EEE configuration");
  }
}
#endif  // VTSS_SW_OPTION_EEE




#ifdef VTSS_SW_OPTION_ICFG
/* ICFG callback functions */
static vtss_rc port_power_savings_global_conf(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
  vtss_port_no_t iport;
  vtss_isid_t isid;

  switch ( req->cmd_mode ) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG: {
#ifdef VTSS_SW_OPTION_EEE
    eee_switch_global_conf_t eee_switch_global_conf;
    VTSS_RC(eee_mgmt_switch_global_conf_get(&eee_switch_global_conf));

    // Print out if not default value (or if requested to print all)
    // Optimize for power
    if (req->all_defaults ||
        (eee_switch_global_conf.optimized_for_power != OPTIMIZE_FOR_POWER_DEFAULT)) {
      VTSS_RC(vtss_icfg_printf(result, "%sgreen-ethernet eee-optimize-for-power\n", eee_switch_global_conf.optimized_for_power ? "" : "no "));
    }
#endif // VTSS_SW_OPTION_EEE
  }
  break;
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

#ifdef VTSS_SW_OPTION_EEE
    eee_switch_conf_t eee_switch_conf;
    eee_switch_state_t eee_switch_state;

    // Get configuration for the current switch
    VTSS_RC(eee_mgmt_switch_conf_get(isid, &eee_switch_conf));
    VTSS_RC(eee_mgmt_switch_state_get(isid, &eee_switch_state));
    if (eee_switch_state.port[iport].eee_capable) {
      if (req->all_defaults ||
          (eee_switch_conf.port[iport].eee_ena != FALSE)) {

        VTSS_RC(vtss_icfg_printf(result, " %sgreen-ethernet eee\n", eee_switch_conf.port[iport].eee_ena ? "" : "no "));
      }

#if EEE_FAST_QUEUES_CNT > 0
      BOOL bool_list[32];
      char queue_buf[200];

      (void)mgmt_ulong2bool_list((ulong) eee_switch_conf.port[iport].eee_fast_queues, &bool_list[0]);
      (void)mgmt_list2txt(&bool_list[0], 0, EEE_FAST_QUEUES_CNT, &queue_buf[0]);

      if (req->all_defaults ||
          (eee_switch_conf.port[iport].eee_fast_queues != 0)) {
        VTSS_RC(vtss_icfg_printf(result, " %sgreen-ethernet eee-urgent-queue %s\n",
                                 eee_switch_conf.port[iport].eee_fast_queues == 0 ? "no " : "",
                                 eee_switch_conf.port[iport].eee_fast_queues == 0 ? "" : queue_buf));
      }
#endif // EEE_FAST_QUEUES_CNT
    }
#endif // VTSS_SW_OPTION_EEE

    // PrefectReach and ActiPhy
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    port_conf_t        port_conf;
    BOOL actiphy_conf_ena;
    BOOL perfect_reach_conf_ena;

    // Get port configuration
    VTSS_RC(port_mgmt_conf_get(isid, iport, &port_conf));

    actiphy_conf_ena = (port_conf.power_mode & VTSS_PHY_POWER_ACTIPHY) >> VTSS_PHY_POWER_ACTIPHY_BIT;
    perfect_reach_conf_ena  = (port_conf.power_mode & VTSS_PHY_POWER_DYNAMIC) >> VTSS_PHY_POWER_DYNAMIC_BIT;
    T_N_PORT(iport, "actiphy_conf_ena:%d, perfect_reach_conf_ena:%d, port_conf.power_mode:%d, VTSS_PHY_POWER_ACTIPHY:%d",
             actiphy_conf_ena, perfect_reach_conf_ena, port_conf.power_mode, VTSS_PHY_POWER_ACTIPHY);

    if (req->all_defaults ||
        (actiphy_conf_ena)) {
      VTSS_RC(vtss_icfg_printf(result, " %sgreen-ethernet energy-detect\n", actiphy_conf_ena ? "" : "no "));
    }

    if (req->all_defaults ||
        (perfect_reach_conf_ena)) {
      VTSS_RC(vtss_icfg_printf(result, " %sgreen-ethernet short-reach\n", perfect_reach_conf_ena ? "" : "no "));
    }
#endif // VTSS_SW_OPTION_PHY_POWER_CONTROL


  default:
    break;
  }

  return VTSS_RC_OK;
}


/* ICFG Initialization function */
vtss_rc eee_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_PHY_POWER_CONTROL_PORT_CONF, port_power_savings_global_conf));
#ifdef VTSS_SW_OPTION_EEE
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_EEE_GLOBAL_CONF, port_power_savings_global_conf));
#endif
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
