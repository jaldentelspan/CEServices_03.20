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
#ifdef VTSS_SW_OPTION_LLDP

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"

#include "cli.h" // For cli_port_iter_init

#include "lldp.h"
#include "lldp_api.h"
#include "lldp_os.h"

#include "msg_api.h"
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

// Help function for printing the TLV information
// In : Session_Id : Session_Id for ICLI_PRINTF
//      entry      : LLDP neighbor entry containing the neighbor information
//      field      : Which TLV field to print
//      mgmt_addr_index : We support multiple management address. Index for which one to print
static void icli_lldp_tlv(u32 session_id, char *name, lldp_remote_entry_t *entry, lldp_tlv_t field, u8 mgmt_addr_index)
{
  i8 string[512];
  ICLI_PRINTF("%-20s: ", name);
  lldp_remote_tlv_to_string(entry, field, &string[0], mgmt_addr_index);
  ICLI_PRINTF("%s\n", string);
}

// See lldp_icli_functions.h
void lldp_icli_status(i32 session_id, BOOL show_neighbors, BOOL show_statistics, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;
  lldp_remote_entry_t *table = NULL, *entry = NULL;
  u16 count;
  u16 i;
  port_iter_t pit;
  BOOL lldp_no_entry_found = TRUE;
  BOOL print_global = TRUE;
  i8   buf[255];
  u8 mgmt_addr_index;


  // Loop through all switches in a stack
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, list)) {
      // Showing neighbor information
      if (show_neighbors) {
        // Start by getting the LLDP neighbor entries
        lldp_mgmt_get_lock();
        count = lldp_remote_get_max_entries();
        table = lldp_mgmt_get_entries(sit.isid); // Get the LLDP entries for the switch in question.

        // Loop though the ports
        (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK);
        while (icli_port_iter_getnext(&pit, list)) {
          // Loop though all the entries containing information
          for (i = 0, entry = table; i < count; i++) {
            // No more entries with information then stop the for loop.
            if (entry == NULL) {
              break;
            }
            // Skip entries not in use or not containing information about the current port
            if ((entry->in_use == 0) || (entry->receive_port != pit.iport)) {
              entry++;
              continue;
            }

            lldp_no_entry_found = FALSE; // Remember that at least one entry was found.

            // This function takes an iport and prints a uport.
            if (lldp_remote_receive_port_to_string(entry->receive_port, buf, sit.isid)) {
              T_D("Part of LAG");
            }

            // Print all the TLVs
            ICLI_PRINTF("%-20s: %s\n", "Local port", buf);
            icli_lldp_tlv(session_id, "Chassis ID", entry, LLDP_TLV_BASIC_MGMT_CHASSIS_ID, 0);
            icli_lldp_tlv(session_id, "Port ID", entry, LLDP_TLV_BASIC_MGMT_PORT_ID, 0);
            icli_lldp_tlv(session_id, "Port Description", entry, LLDP_TLV_BASIC_MGMT_PORT_DESCR, 0);
            icli_lldp_tlv(session_id, "System Name", entry, LLDP_TLV_BASIC_MGMT_SYSTEM_NAME, 0);
            icli_lldp_tlv(session_id, "System Description", entry, LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR, 0);
            icli_lldp_tlv(session_id, "System Capabilities", entry, LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA, 0);

            // Printing management address if present
            for (mgmt_addr_index = 0; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
              if (entry->mgmt_addr[mgmt_addr_index].length > 0) {
                icli_lldp_tlv(session_id, "Management Address", entry, LLDP_TLV_BASIC_MGMT_MGMT_ADDR, mgmt_addr_index);
              }
            }
#ifdef VTSS_SW_OPTION_POE
            lldp_remote_poeinfo2string(entry, &buf[0]);
            ICLI_PRINTF("%-20s: %s\n", "Power Over Ethernet", buf);
#endif
            ICLI_PRINTF("\n");

            // Select next entry
            if (entry != NULL) {
              entry++;
            }
          }
        }
        lldp_mgmt_get_unlock();

        // If no entries with valid information were found then tell the user.
        if (lldp_no_entry_found) {
          ICLI_PRINTF("No LLDP entries found \n");
        }
      } // End show neighbors


      // Don't print global information if user has specified at specific interface.
      if (has_interface) {
        print_global = FALSE;
      }


      // showing  statistics
      if (show_statistics) {

        // Get the statistics
        lldp_counters_rec_t stat_table[LLDP_PORTS], *stat;

        if (lldp_mgmt_stat_get(sit.isid, &stat_table[0], NULL, NULL)) {
          T_W("Problem getting counters");
        }

        // Printing global information
        if (print_global) {
          lldp_mib_stats_t global_cnt;
          time_t           last_change_ago;
          char             last_change_str[255];

          (void)lldp_mgmt_stat_get(VTSS_ISID_START /* anything valid (doesn't have to exist) */, NULL, &global_cnt, &last_change_ago);
          lldp_mgmt_last_change_ago_to_str(last_change_ago, &last_change_str[0]);

          // Global counters //
          ICLI_PRINTF("LLDP global counters\n");

          ICLI_PRINTF("Neighbor entries was last changed at %s.\n", last_change_str);

          ICLI_PRINTF("Total Neighbors Entries Added    %d.\n", global_cnt.table_inserts);
          ICLI_PRINTF("Total Neighbors Entries Deleted  %d.\n", global_cnt.table_deletes);
          ICLI_PRINTF("Total Neighbors Entries Dropped  %d.\n", global_cnt.table_drops);
          ICLI_PRINTF("Total Neighbors Entries Aged Out %d.\n", global_cnt.table_ageouts);

          // Local counters //
          ICLI_PRINTF("\nLLDP local counters\n");
          ICLI_PRINTF("       Rx       Tx       Rx       Rx       Rx TLV   Rx TLV   Rx TLV\n");
          ICLI_PRINTF("Port   Frames   Frames   Errors   Discards Errors   Unknown  Organiz.  Aged\n");
          ICLI_PRINTF("----   ------   ------   ------   -------- ------   -------  -------  -----\n");


#ifdef VTSS_SW_OPTION_AGGR
          aggr_mgmt_group_member_t glag_members;
          vtss_glag_no_t glag_no;
          // Insert the statistic for the GLAGs. The lowest port number in the GLAG collection contains statistic (See also packet_api.h).
          for (glag_no = AGGR_MGMT_GROUP_NO_START; glag_no < AGGR_MGMT_GROUP_NO_END; glag_no++) {
            // Get the port members
            T_D("Get glag members, isid:%d, glag_no:%u", sit.isid, glag_no);
            (void) aggr_mgmt_members_get(sit.isid, glag_no, &glag_members, FALSE);
            vtss_port_no_t      iport_number;
            // Loop through all ports. Stop at first port that is part of the GLAG.
            for (iport_number = VTSS_PORT_NO_START; iport_number < VTSS_PORT_NO_END; iport_number++) {

              if (iport_number < LLDP_PORTS) { // Make sure that we don't index array out-of-bounds.
                if (glag_members.entry.member[iport_number]) {
                  char lag_string[50] = "";
                  if (glag_no >= AGGR_MGMT_GLAG_START) {
                    sprintf(lag_string, "GLAG %u", glag_no - AGGR_MGMT_GLAG_START + 1);
                  } else {
                    sprintf(lag_string, "LLAG %u", glag_no - AGGR_MGMT_GROUP_NO_START + 1);
                  }

                  stat = &stat_table[iport_number - VTSS_PORT_NO_START];
                  ICLI_PRINTF("%-6s %-8d %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
                              lag_string,
                              stat->rx_total,
                              stat->tx_total,
                              stat->rx_error,
                              stat->rx_discarded,
                              stat->TLVs_discarded,
                              stat->TLVs_unrecognized,
                              stat->TLVs_org_discarded,
                              stat->ageouts);

                  break;
                }
              }
            }
          }
#endif // VTSS_SW_OPTION_AGGR
        } // end print_global


        // loop through the ports.
        (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK);
        while (icli_port_iter_getnext(&pit, list)) {

          // Check if the port is part of a LAG
          // This function takes an iport and prints a uport.
          if (lldp_remote_receive_port_to_string(pit.iport, buf, sit.isid) == 1) {
            ICLI_PRINTF("%-6lu %-8s%s\n",
                        pit.uport,
                        "Part of aggr ",
                        buf);
          } else {
            stat = &stat_table[pit.iport];
            ICLI_PRINTF("%-6lu %-8d %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
                        pit.uport,
                        stat->rx_total,
                        stat->tx_total,
                        stat->rx_error,
                        stat->rx_discarded,
                        stat->TLVs_discarded,
                        stat->TLVs_unrecognized,
                        stat->TLVs_org_discarded,
                        stat->ageouts);
          }
        } // End while icli_stack_port_iter_getnext
      } // End show statistics
      print_global = FALSE; // Only print global information once.
    } // end icli_stack_isid_iter_getnext
  }
}


// Function for clearing the statistics table
void lldp_icli_clear_counters(void)
{
  vtss_isid_t isid;
  // loop though all switches and clear the statistics if a switch is found
  for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
    if (msg_switch_exists(isid)) {
      lldp_mgmt_stat_clr(isid);
    }
  }
}


// See lldp_icli_functions.h
void lldp_icli_global_conf(BOOL holdtime, BOOL tx_interval, BOOL reinit, u16 new_val, BOOL no)
{
  lldp_struc_0_t      lldp_conf;

  // Get current configuration
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  // holdtime
  if (holdtime) {
    if (no) {
      lldp_conf.msgTxHold = LLDP_TX_HOLD_DEFAULT;
    } else {
      lldp_conf.msgTxHold = new_val;
    }

  }

  // interval
  if (tx_interval) {
    if (no) {
      lldp_conf.msgTxInterval = LLDP_TX_INTERVAL_DEFAULT;
    } else {
      lldp_conf.msgTxInterval = new_val;
    }
  }

  // Reinit
  if (reinit) {
    if (no) {
      lldp_conf.reInitDelay = LLDP_REINIT_DEFAULT;
    } else {
      lldp_conf.reInitDelay = new_val;
    }
  }

  // Set new configuration.
  if (lldp_mgmt_set_config(&lldp_conf, VTSS_ISID_LOCAL) != VTSS_RC_OK) {
    T_W("Could not set LLDP configuration");
  }
}

// See lldp_icli_functions.h
void lldp_icli_tlv_select(BOOL mgmt, BOOL port, BOOL sys_capa, BOOL sys_des, BOOL sys_name, BOOL no)
{
  lldp_struc_0_t      lldp_conf;
  vtss_port_no_t      iport;
  vtss_isid_t         isid;

  // For ICLI we enables/disable optional TLVs globally while we do it per port in normal, so we loop through all switches and ports and sets the optional TLVs to same for all ports.

  T_DG(TRACE_GRP_CONF, "mgmt:%d, port:%d, sys_capa:%d, sys_des:%d, sys_name:%d, no:%d", mgmt, port, sys_capa, sys_des, sys_name, no);

  // Loop through all switches in a stack
  for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
    // Get current configuration
    lldp_mgmt_get_config(&lldp_conf, isid);

    // Update which optional TLVs to transmit
    if (msg_switch_exists(isid)) {
      for (iport = 0; iport < LLDP_PORTS; iport++) {
        if (mgmt) {
          lldp_os_set_optional_tlv(LLDP_TLV_BASIC_MGMT_MGMT_ADDR,
                                   !no, &lldp_conf, iport);
        }
        if (port) {
          lldp_os_set_optional_tlv(LLDP_TLV_BASIC_MGMT_PORT_DESCR,
                                   !no, &lldp_conf, iport);
        }

        if (sys_capa) {
          lldp_os_set_optional_tlv(LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA,
                                   !no, &lldp_conf, iport);
        }

        if (sys_des) {
          lldp_os_set_optional_tlv(LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR,
                                   !no, &lldp_conf, iport);
        }

        if (sys_name) {
          lldp_os_set_optional_tlv(LLDP_TLV_BASIC_MGMT_SYSTEM_NAME,
                                   !no, &lldp_conf, iport);
        }

      }

      // Set new configuration.
      if (lldp_mgmt_set_config(&lldp_conf, isid) != VTSS_RC_OK) {
        T_W("Could not set LLDP configuration");
      }
    }
  }
}

// See lldp_icli_functions.h
void lldp_icli_mode(icli_stack_port_range_t *list, BOOL tx, BOOL rx, BOOL no)
{
  vtss_isid_t isid;

  // Just making sure that we don't access NULL pointer.
  if (list != NULL) {
    isid = topo_usid2isid(list->switch_range[0].usid);
  } else {
    isid = VTSS_ISID_LOCAL;
  }
  port_iter_t pit;
  lldp_struc_0_t      lldp_conf;

  // Get current configuration
  lldp_mgmt_get_config(&lldp_conf, isid);

  // Loop through ports.
  if (icli_port_iter_init(&pit, isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, list)) {

      // We are using one state containing the whole mode configuration, so we need to convert the separate tx/rx.
      T_IG_PORT(TRACE_GRP_CLI, pit.iport, "tx:%d, no:%d, rx:%d, admin:%d", tx, no, rx, lldp_conf.admin_state[pit.iport]);
      switch (lldp_conf.admin_state[pit.iport]) {
      case LLDP_DISABLED:
        if (tx && !no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_TX_ONLY;
        }

        if (rx && !no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_RX_ONLY;
        }
        break;
      case LLDP_ENABLED_RX_ONLY:
        if (rx && no) {
          lldp_conf.admin_state[pit.iport] = LLDP_DISABLED;
        }

        if (tx && !no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_RX_TX;
        }
        break;
      case LLDP_ENABLED_TX_ONLY:
        if (rx && !no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_RX_TX;
        }

        if (tx && no) {
          lldp_conf.admin_state[pit.iport] = LLDP_DISABLED;
        }
        break;

      case LLDP_ENABLED_RX_TX:
        if (rx && no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_TX_ONLY;
        }

        if (tx && no) {
          lldp_conf.admin_state[pit.iport] = LLDP_ENABLED_RX_ONLY;
        }
        break;
      }
    }
  }
  // Set new configuration.
  if (lldp_mgmt_set_config(&lldp_conf, isid) != VTSS_RC_OK) {
    T_W("Could not set LLDP configuration");
  }
}

//
// ICFG (Show running)
//
#ifdef VTSS_SW_OPTION_ICFG

// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      iport - Port in question
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static vtss_rc lldp_icfg_print_mode(lldp_struc_0_t *lldp_conf, vtss_isid_t isid, vtss_port_no_t iport, BOOL all_defaults, vtss_icfg_query_result_t *result)
{
  // Since we work with a single state containing both RX and TX we need to figure out if rx and tx is enabled.
  // The default value is different depending upon if LLDP-MED is included or not, so that has to be taking into account as well
  BOOL tx_en = FALSE;
  BOOL rx_en = FALSE;
  BOOL print_tx = FALSE;
  BOOL print_rx = FALSE;

  // Get current configuration for this switch
  lldp_mgmt_get_config(lldp_conf, isid);

  /*lint --e{506} ... yes, LLDP_ADMIN_STATE_DEFAULT is constant! */
  switch (lldp_conf->admin_state[iport]) {
  case LLDP_ENABLED_RX_ONLY:
    rx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT == LLDP_DISABLED || all_defaults) {
      print_rx = TRUE;
    }
    break;
  case LLDP_ENABLED_TX_ONLY:
    tx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT == LLDP_DISABLED) {
      print_tx = TRUE;
    }
    break;
  case LLDP_ENABLED_RX_TX:
    tx_en = TRUE;
    rx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT == LLDP_DISABLED) {
      print_tx = TRUE;
      print_rx = TRUE;
    }
    break;
  case LLDP_DISABLED:
    if (LLDP_ADMIN_STATE_DEFAULT != LLDP_DISABLED) {
      print_tx = TRUE;
      print_rx = TRUE;
    }
    break;
  }

  T_DG_PORT(TRACE_GRP_CLI, iport, "tx_en:%d, rx_en:%d, print_rx:%d, print_tx:%d, admin:%d",
            tx_en, rx_en, print_rx, print_rx, lldp_conf->admin_state[iport]);
  if (all_defaults ||
      print_rx) {
    VTSS_RC(vtss_icfg_printf(result, " %slldp receive\n", rx_en ? "" : "no "));
  }

  if (all_defaults ||
      print_tx) {
    VTSS_RC(vtss_icfg_printf(result, " %slldp transmit\n", tx_en ? "" : "no "));
  }
  return VTSS_RC_OK;
}

// Help function for print global configuration.
// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      iport - Port in question
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static vtss_rc lldp_icfg_print_global(lldp_struc_0_t *lldp_conf, vtss_port_no_t iport, BOOL all_defaults, vtss_icfg_query_result_t *result)
{
  BOOL optional_tlv_mgmt = lldp_mgmt_get_opt_tlv_enabled(LLDP_TLV_BASIC_MGMT_MGMT_ADDR, iport, VTSS_ISID_LOCAL);
  BOOL optional_tlv_port = lldp_mgmt_get_opt_tlv_enabled(LLDP_TLV_BASIC_MGMT_PORT_DESCR, iport, VTSS_ISID_LOCAL);
  BOOL optional_tlv_sys_name = lldp_mgmt_get_opt_tlv_enabled(LLDP_TLV_BASIC_MGMT_SYSTEM_NAME, iport, VTSS_ISID_LOCAL);
  BOOL optional_tlv_sys_des = lldp_mgmt_get_opt_tlv_enabled(LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR, iport, VTSS_ISID_LOCAL);
  BOOL optional_tlv_sys_capa = lldp_mgmt_get_opt_tlv_enabled(LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA, iport, VTSS_ISID_LOCAL);


  // TX hold
  if (all_defaults ||
      (lldp_conf->msgTxHold != LLDP_TX_HOLD_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp holdtime %d\n", lldp_conf->msgTxHold));
  }

  // Timer
  if (all_defaults ||
      (lldp_conf->msgTxInterval != LLDP_TX_INTERVAL_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp timer %d\n", lldp_conf->msgTxInterval));
  }

  // reinit delay
  if (all_defaults ||
      (lldp_conf->reInitDelay != LLDP_REINIT_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp reinit %d\n", lldp_conf->reInitDelay));
  }


  if (all_defaults ||
      optional_tlv_mgmt == FALSE) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp tlv-select management-address\n", optional_tlv_mgmt ? "" : "no "));
  }

  if (all_defaults ||
      optional_tlv_port == FALSE) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp tlv-select port-description\n", optional_tlv_port ? "" : "no "));
  }

  if (all_defaults ||
      optional_tlv_sys_capa == FALSE) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp tlv-select system-capabilities\n", optional_tlv_sys_capa ? "" : "no "));
  }

  if (all_defaults ||
      optional_tlv_sys_name == FALSE) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp tlv-select system-name\n", optional_tlv_sys_name ? "" : "no "));
  }

  if (all_defaults ||
      optional_tlv_sys_des == FALSE) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp tlv-select system-description\n", optional_tlv_sys_des ? "" : "no "));
  }
  return VTSS_RC_OK;
}

/* ICFG callback functions */
static vtss_rc lldp_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
  vtss_isid_t         isid;
  lldp_struc_0_t      lldp_conf;
  vtss_port_no_t      iport;

  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    // Get current configuration for this switch
    lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

    // Any port can be used
    VTSS_RC(lldp_icfg_print_global(&lldp_conf, 0, req->all_defaults, result));
    break;
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);
    T_D("Isid:%d, iport:%u, req->instance_id.port.usid:%d", isid, iport, req->instance_id.port.usid);

    if (msg_switch_exists(isid)) {
      // Get current configuration for this switch
      lldp_mgmt_get_config(&lldp_conf, isid);

      VTSS_RC(lldp_icfg_print_mode(&lldp_conf, isid, iport, req->all_defaults, result));
      break;

    default:
      //Not needed for LLDP
      break;
    }
  } // End msg_switch_exists
  return VTSS_RC_OK;
}

/* ICFG Initialization function */
vtss_rc lldp_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDP_GLOBAL_CONF, lldp_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDP_PORT_CONF, lldp_icfg_global_conf));

  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG
#endif // #ifdef VTSS_SW_OPTION_LLDP
/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
