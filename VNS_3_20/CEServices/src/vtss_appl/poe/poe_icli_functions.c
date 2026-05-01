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
#ifdef VTSS_SW_OPTION_POE

#include "poe_api.h"
#include "poe_custom_api.h"
#include "cli_trace_def.h"

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"

#include "icfg_api.h" // For vtss_icfg_query_request_t
#include "misc_api.h" // for uport2iport
#include "msg_api.h" // For msg_switch_exists
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

#include "mgmt_api.h" //mgmt_str_float2long

// Function for checking is if PoE is supported for a specific port. If PoE isn't supported for the port, a printout is done.
// In : session_id - session_id of ICLI_PRINTF
//      iport - Internal port
//      uport - User port number
//      isid  - Internal switch id.
// Return: TRUE if PoE chipset is found for the iport, else FALSE
static BOOL is_poe_supported(i32 session_id, vtss_port_no_t iport, vtss_port_no_t uport, vtss_isid_t isid, poe_chipset_t *poe_chip_found)
{
  if (poe_chip_found[iport] == NO_POE_CHIPSET_FOUND) {
    ICLI_PRINTF("%-5lu PoE not supported \n", uport);
    return FALSE;
  } else {
    return TRUE;
  }
}

// Help function for printing out status
// In : session_id - Session_Id for ICLI_PRINTF
//      debug  - Set to TRUE in order to get more PoE information printed
//      has_interface - TRUE if user has specified a specific interface
//      list - List of interfaces (ports)
static void poe_status(i32 session_id, vtss_isid_t isid, BOOL debug, BOOL has_interface, icli_stack_port_range_t *list)
{
  port_iter_t        pit;
  poe_status_t       status;
  char               txt_string[50];
  char               firmware_string[50];
  char               class_string[10];

  if (debug) {
    ICLI_PRINTF("I2C Addr.  PoE chipset found           Firmware Info  Port  PD  Class Port Status                              Power Used [W]  Current Used [mA]\n");
  } else {
    ICLI_PRINTF("Port  PD Class  Port Status                              Power Used [W]  Current Used [mA]\n");
    ICLI_PRINTF("----  -- -----  ---------------------------------------- --------------  -----------------\n");
  }

  poe_mgmt_get_status(isid, &status); // Update the status fields

  poe_local_conf_t poe_local_conf;
  poe_mgmt_get_local_config(&poe_local_conf, isid);

  poe_chipset_t poe_chip_found[VTSS_PORTS];
  poe_mgmt_is_chip_found(isid, &poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  // Loop through all front ports
  if (icli_port_iter_init(&pit, isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, list)) {
      if (debug) {
        poe_custom_entry_t hw_conf;

        (void) poe_custom_get_hw_config(pit.iport, &hw_conf);
        ICLI_PRINTF("0x%-8X %-27s %-15s",
                    hw_conf.i2c_addr[poe_chip_found[pit.iport]],
                    poe_chipset2txt(poe_chip_found[pit.iport], &txt_string[0]),
                    poe_mgmt_firmware_info_get(pit.iport, &firmware_string[0]));
      }


      if (is_poe_supported(session_id, pit.iport, pit.uport, isid, &poe_chip_found[0])) {
        ICLI_PRINTF("%-5lu %-8s %-41s %-16s %-19d \n",
                    pit.uport,
                    poe_class2str(status, pit.iport, &class_string[0]),
                    poe_status2str(status.port_status[pit.iport], pit.iport, &poe_local_conf),
                    one_digi_float2str(status.power_used[pit.iport], &txt_string[0]),
                    status.current_used[pit.iport]);
      }
    }
  }
}


// See poe_icli_functions.h
void poe_icli_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;

  // Loop all the switches in question.
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, list)) {
      poe_status(session_id, sit.isid, FALSE, has_interface, list);
    }
  }
}


// See poe_icli_functions.h
void poe_icli_priority_conf(BOOL has_low, BOOL has_high, BOOL has_critical, vtss_port_no_t iport, BOOL no, poe_local_conf_t *poe_local_conf)
{
  // Update mode
  if (has_low) {
    poe_local_conf->priority[iport] = LOW;
  } else if (has_high) {
    poe_local_conf->priority[iport] = HIGH;
  } else if (has_critical) {
    poe_local_conf->priority[iport] = CRITICAL;
  } else if (no) {
    poe_local_conf->priority[iport] = POE_PRIORITY_DEFAULT;
  }
}

// Help function for setting poe mode
// In : has_poe - TRUE is PoE port power shall be poe mode
//      has_poe_plus - TRUE if PoE port power shall be poe+ mode.
//      iport - Port in question
//      no - TRUE if mode shall be set to default.
//      poe_local_conf - Pointer to the current configuration.
static void poe_icli_mode_conf(BOOL has_poe, BOOL has_poe_plus, vtss_port_no_t iport, BOOL no, poe_local_conf_t *poe_local_conf)
{
  // Update mode
  if (has_poe) {
    poe_local_conf->poe_mode[iport] = POE_MODE_POE;
  } else if (has_poe_plus) {
    poe_local_conf->poe_mode[iport] = POE_MODE_POE_PLUS;
  } else if (no) {
    poe_local_conf->poe_mode[iport] = POE_MODE_DEFAULT;
  }
}


// Type used for selecting which poe icli configuration to update for the common function.
typedef enum {VTSS_POE_ICLI_MODE,
              VTSS_POE_ICLI_PRIORITY,
              VTSS_POE_ICLI_POWER_LIMIT
             } vtss_poe_icli_conf_t; // Which configuration do to



// Help function for setting power limit
// In : Session_Id - session_id for ICLI_PRINTF
//      iport - Internal port in question
//      uport - User port id
//      value_str - new value (as string)
//      no - TRUE if mode shall be set to default.
//      poe_local_conf - Pointer to the current configuration.

static void poe_icli_power_limit_conf(i32 session_id, vtss_port_no_t iport, vtss_port_no_t uport, char *value_str, poe_local_conf_t *poe_local_conf, BOOL no)
{
  u16 port_power;
  char txt_string[50];
  long value;

  if (no) {
    value = POE_MAX_POWER_DEFAULT;
  } else {
    if (mgmt_str_float2long(value_str, &value, 0, poe_max_power_mode_dependent(iport, poe_local_conf->poe_mode[iport]), 1) != VTSS_RC_OK) {
      ICLI_PRINTF("Invalid value:%s", value_str);
      return;
    }
  }

  T_D("Setting max port power to %ld, poe_max_power_mode_dependent = %d",
      value, poe_max_power_mode_dependent(iport, poe_local_conf->poe_mode[iport]));

  if (value > poe_max_power_mode_dependent(iport, poe_local_conf->poe_mode[iport])) {
    port_power = poe_max_power_mode_dependent(iport, poe_local_conf->poe_mode[iport]);
    mgmt_long2str_float(&txt_string[0], port_power, 1);
    ICLI_PRINTF("Maximum allowed power (for the current mode) for port %lu is limited to %s W\n", uport, txt_string);
  } else {
    port_power = value;
  }

  poe_local_conf->max_port_power[iport] = port_power;
}


// Common function used for setting interface configurations.
// In : session_id - Session_Id for ICLI_PRINTF
//      poe_conf_type - Selecting which configuration to update.
//      param* - List of parameters. Depending upon which poe_conf_type is selected.
//      plist - List of interfaces to update
//      no - TRUE if user wants to set configuration to default
static void poe_icli_common(i32 session_id, vtss_poe_icli_conf_t poe_conf_type,
                            BOOL param1, BOOL param2, BOOL param3,
                            icli_stack_port_range_t *plist, BOOL no, char *value)
{
  poe_local_conf_t poe_local_conf;
  vtss_isid_t isid;

  // Just making sure that we don't access NULL pointer.
  if (plist != NULL) {
    isid = topo_usid2isid(plist->switch_range[0].usid);
  } else {
    isid = VTSS_ISID_LOCAL;
  }

  // Get current configuration
  poe_mgmt_get_local_config(&poe_local_conf, isid);

  poe_chipset_t poe_chip_found[VTSS_PORTS];
  poe_mgmt_is_chip_found(isid, &poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  port_iter_t        pit;
  if (icli_port_iter_init(&pit, isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, plist)) {
      // Ignore if PoE isn't supported for this port.
      if (is_poe_supported(session_id, pit.iport, pit.uport, isid, &poe_chip_found[0])) {
        switch  (poe_conf_type) {
        case VTSS_POE_ICLI_MODE:
          poe_icli_mode_conf(param1, param2, pit.iport, no, &poe_local_conf);
          break;
        case VTSS_POE_ICLI_PRIORITY:
          poe_icli_priority_conf(param1, param2, param3, pit.iport, no, &poe_local_conf);
          break;

        case VTSS_POE_ICLI_POWER_LIMIT:
          poe_icli_power_limit_conf(session_id, pit.iport, pit.uport, value, &poe_local_conf, no);
          break;

        default:
          T_E("Unknown poe_conf_type:%d", poe_conf_type);
        }
      }
    }
    // Set new configuration
    poe_mgmt_set_local_config(&poe_local_conf, isid);
  }
}
// See poe_icli_functions.h
void poe_icli_power_supply(i32 session_id, vtss_usid_t usid, u32 value, BOOL no)
{
  // Get current configuration
  poe_local_conf_t poe_local_conf;
  vtss_isid_t isid = topo_usid2isid(usid);
  poe_mgmt_get_local_config(&poe_local_conf, isid);

  if (no) {
    poe_local_conf.primary_power_supply = POE_POWER_SUPPLY_MAX;
  } else {
    poe_local_conf.primary_power_supply = value;
  }
  // Set new configuration
  poe_mgmt_set_local_config(&poe_local_conf, isid);
}

// See poe_icli_functions.h
void poe_icli_management_mode(i32 session_id, BOOL has_class_consumption, BOOL has_class_reserved_power, BOOL has_allocation_consumption, BOOL has_alllocation_reserved_power, BOOL has_lldp_consumption, BOOL has_lldp_reserved_power, BOOL no)
{

  poe_master_conf_t poe_master_conf;
  poe_mgmt_get_master_config(&poe_master_conf);

  if (has_class_consumption) {
    poe_master_conf.power_mgmt_mode = CLASS_CONSUMP;
  } else if (has_class_reserved_power) {
    poe_master_conf.power_mgmt_mode = CLASS_RESERVED;
  } else if (has_alllocation_reserved_power) {
    poe_master_conf.power_mgmt_mode = ALLOCATED_RESERVED;
  } else if (has_allocation_consumption) {
    poe_master_conf.power_mgmt_mode = ALLOCATED_CONSUMP;
  } else if (has_lldp_reserved_power) {
    poe_master_conf.power_mgmt_mode = LLDPMED_RESERVED;
  } else if (has_lldp_consumption) {
    poe_master_conf.power_mgmt_mode = LLDPMED_CONSUMP;
  }

  if (no) {
    poe_master_conf.power_mgmt_mode = POE_MGMT_MODE_DEFAULT;
  }

  poe_mgmt_set_master_config(&poe_master_conf);
}

// See poe_icli_functions.h
void poe_icli_priority(i32 session_id, BOOL has_low, BOOL has_high, BOOL has_critical, icli_stack_port_range_t *plist, BOOL no)
{
  poe_icli_common(session_id, VTSS_POE_ICLI_PRIORITY, has_low, has_high, has_critical, plist, no, "");
}

// See poe_icli_functions.h
void poe_icli_mode(i32 session_id, BOOL has_poe, BOOL has_poe_plus, icli_stack_port_range_t *plist, BOOL no)
{
  poe_icli_common(session_id, VTSS_POE_ICLI_MODE, has_poe, has_poe_plus, FALSE, plist, no, "");
}

// See poe_icli_functions.h
void poe_icli_power_limit(i32 session_id, char *value, icli_stack_port_range_t *plist, BOOL no)
{
  poe_icli_common(session_id, VTSS_POE_ICLI_POWER_LIMIT, FALSE, FALSE, FALSE, plist, no, value);
}

//
// ICFG (Show running)
//
#ifdef VTSS_SW_OPTION_ICFG

// Function called by ICFG.
static vtss_rc poe_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
  vtss_isid_t      isid;
  vtss_port_no_t   iport;
  poe_local_conf_t poe_local_conf;

  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    poe_mgmt_get_local_config(&poe_local_conf, VTSS_ISID_LOCAL);

    //
    // Power management mode
    //
    if (req->all_defaults ||
        poe_local_conf.power_mgmt_mode != POE_MGMT_MODE_DEFAULT) {

      if (poe_local_conf.power_mgmt_mode == POE_MGMT_MODE_DEFAULT) {
        VTSS_RC(vtss_icfg_printf(result, "no poe management mode\n"));
      } else {
        VTSS_RC(vtss_icfg_printf(result, "poe management mode %s\n",
                                 poe_local_conf.power_mgmt_mode == CLASS_CONSUMP ? "class-consumption" :
                                 poe_local_conf.power_mgmt_mode == CLASS_RESERVED ? "class-reserved-power" :
                                 poe_local_conf.power_mgmt_mode == ALLOCATED_CONSUMP ? "allocation-consumption" :
                                 poe_local_conf.power_mgmt_mode == ALLOCATED_RESERVED ? "allocation-reserved-power" :
                                 poe_local_conf.power_mgmt_mode == LLDPMED_CONSUMP ? "lldp-consumption" :
                                 poe_local_conf.power_mgmt_mode == LLDPMED_RESERVED ? "lldp-reserved-power" :
                                 "Unknown PoE management mode"));
      }
    }

    //
    // Power supply
    //
    if (req->all_defaults ||
        poe_local_conf.primary_power_supply != POE_POWER_SUPPLY_MAX) {

      if (poe_local_conf.primary_power_supply == POE_POWER_SUPPLY_MAX) {
        VTSS_RC(vtss_icfg_printf(result, "no poe supply\n"));
      } else {
        VTSS_RC(vtss_icfg_printf(result, "poe supply %d\n", poe_local_conf.primary_power_supply));
      }
    }
    break;
    //
    // Interface configurations
    //
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

    if (msg_switch_exists(isid)) {
      // Get current configuration
      poe_mgmt_get_local_config(&poe_local_conf, isid);

      //
      // Mode
      //
      if (req->all_defaults ||
          poe_local_conf.poe_mode[iport] != POE_MODE_DEFAULT) {
        if (poe_local_conf.poe_mode[iport] == POE_MODE_DEFAULT) {
          // No command
          VTSS_RC(vtss_icfg_printf(result, " no poe mode\n"));
        } else {
          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe mode %s\n",
                                   poe_local_conf.poe_mode[iport] == POE_MODE_POE_PLUS ? "poe-plus" :
                                   poe_local_conf.poe_mode[iport] == POE_MODE_POE ? "poe" :
                                   poe_local_conf.poe_mode[iport] == POE_MODE_POE_DISABLED ? "disabled" :
                                   "Unknown PoE mode"));
        }
      }

      //
      // Priority
      //
      if (req->all_defaults ||
          poe_local_conf.priority[iport] != POE_PRIORITY_DEFAULT) {

        if (poe_local_conf.priority[iport] == POE_PRIORITY_DEFAULT) {
          // No command
          VTSS_RC(vtss_icfg_printf(result, " no poe priority\n"));
        } else {
          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe priority %s\n",
                                   poe_local_conf.priority[iport] == LOW ? "low" :
                                   poe_local_conf.priority[iport] == HIGH ? "high" :
                                   poe_local_conf.priority[iport] == CRITICAL ? "critical" :
                                   "Unknown PoE priority"));
        }
      }

      //
      // Maximum power
      //
      if (req->all_defaults ||
          poe_local_conf.max_port_power[iport] != POE_MAX_POWER_DEFAULT) {

        if (poe_local_conf.max_port_power[iport] == POE_MAX_POWER_DEFAULT) {
          // No command
          VTSS_RC(vtss_icfg_printf(result, " no poe power limit\n"));
        } else {
          // Normal command
          char txt_string[50];
          mgmt_long2str_float(&txt_string[0], poe_local_conf.max_port_power[iport], 1);
          VTSS_RC(vtss_icfg_printf(result, " poe power limit %s\n", txt_string));
        }
      }
    }
    break;
  default:
    //Not needed for PoE
    break;
  }

  return VTSS_RC_OK;
}


/* ICFG Initialization function */
vtss_rc poe_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_POE_GLOBAL_CONF, poe_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_POE_PORT_CONF, poe_icfg_global_conf));
  return VTSS_RC_OK;
}

#endif // VTSS_SW_OPTION_ICFG
#endif //VTSS_SW_OPTION_POE
