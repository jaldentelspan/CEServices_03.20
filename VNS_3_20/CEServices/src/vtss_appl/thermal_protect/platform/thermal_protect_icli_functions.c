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
#ifdef VTSS_SW_OPTION_THERMAL_PROTECT

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"

#include "cli.h" // For cli_port_iter_init

#include "thermal_protect.h"
#include "thermal_protect_api.h"

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
// See thermal_protect_icli_functions.h
void thermal_protect_status(i32 session_id, BOOL has_interface, icli_stack_port_range_t *port_list)
{
  thermal_protect_local_status_t   switch_status;
  vtss_rc rc;
  switch_iter_t sit;
  port_iter_t pit;
  i8 str_buf[200];


  // Loop through all switches in stack
  // Loop all the switches in question.
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, port_list)) {
      // Print out of status data
      ICLI_PRINTF("Port  Chip Temp.  Port Status\n");
      if ((rc = thermal_protect_mgmt_get_switch_status(&switch_status, sit.isid))) {
        ICLI_PRINTF("%s", error_txt(rc));
      } else {
        // Loop through all ports
        if (icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK) == VTSS_RC_OK) {
          while (icli_port_iter_getnext(&pit, port_list)) {
            sprintf(str_buf, "%-5u %d %-8s %s", pit.uport, switch_status.port_temp[pit.iport], "C",
                    thermal_protect_power_down2txt(switch_status.port_powered_down[pit.iport]));
            ICLI_PRINTF("%-12s \n", str_buf);
          }
        }
      }
    }
  }
}



// See thermal_protect_icli_functions.h
void thermal_protect_prio(icli_stack_port_range_t *port_list, u8 prio, BOOL no)
{
  thermal_protect_local_conf_t     thermal_protect_local_conf;
  vtss_rc rc;
  switch_iter_t sit;
  port_iter_t pit;

  // Loop through all switches in stack
  (void) icli_switch_iter_init(&sit);
  while (icli_switch_iter_getnext(&sit, port_list)) {

    // Get configuration for the current switch
    thermal_protect_mgmt_get_switch_conf(&thermal_protect_local_conf, sit.isid);

    // Loop through all ports
    (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK);
    while (icli_port_iter_getnext(&pit, port_list)) { // !interface is used to get all ports in case no interface is specified.

      // Set to default if this is the no command
      if (no) {
        thermal_protect_local_conf.port_prio[pit.iport] = 0;
      } else {
        thermal_protect_local_conf.port_prio[pit.iport] = prio;
      }
    }
  }


  // Set the new configuration
  if ((rc = thermal_protect_mgmt_set_switch_conf(&thermal_protect_local_conf, sit.isid)) != VTSS_OK) {
    CPRINTF("%s \n", error_txt(rc));
  }
}



// See thermal_protect_icli_functions.h
void thermal_protect_temp(i32 session_id, icli_unsigned_range_t *prio_list, i8 new_temp, BOOL no)
{
  u8 element_index;
  u8 prio_index;
  switch_iter_t sit;
  thermal_protect_local_conf_t     thermal_protect_local_conf;
  vtss_rc rc;

  // prio_list can be NULL if the uses simply want to set all priorities
  if (prio_list != NULL) {

    // Loop through all switches in stack
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
    while (switch_iter_getnext(&sit)) {

      // Get configuration for the current switch
      thermal_protect_mgmt_get_switch_conf(&thermal_protect_local_conf, sit.isid);

      // Loop through the elements in the list
      for (element_index = 0; element_index < prio_list->cnt; element_index++) {
        for (prio_index = prio_list->range[element_index].min; prio_index <= prio_list->range[element_index].max; prio_index++) {
          // Make sure we don't get out of bounce
          if (prio_index > THERMAL_PROTECT_PRIOS_MAX) {
            ICLI_PRINTF("%% Ignoring invalid priority:%d. Valid range is %lu-%lu\n",
                        prio_index, THERMAL_PROTECT_PRIOS_MIN, THERMAL_PROTECT_PRIOS_MAX);
            continue;
          }

          // Set to default if this is the no command
          if (no) {
            thermal_protect_local_conf.glbl_conf.prio_temperatures[prio_index] = THERMAL_PROTECT_TEMP_MAX;
          } else {
            thermal_protect_local_conf.glbl_conf.prio_temperatures[prio_index] = new_temp;
          }

        }
      }

      // Set the new configuration
      if ((rc = thermal_protect_mgmt_set_switch_conf(&thermal_protect_local_conf, sit.isid)) != VTSS_OK) {
        ICLI_PRINTF("%s \n", error_txt(rc));
      }
    }
  }
}




#ifdef VTSS_SW_OPTION_ICFG

/* ICFG callback functions */
static vtss_rc thermal_protect_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
  u8 prio_index;
  thermal_protect_local_conf_t     thermal_protect_local_conf;
  vtss_port_no_t iport;
  T_N("req->cmd_mode:%d", req->cmd_mode);
  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    // Get configuration for the current switch
    thermal_protect_mgmt_get_switch_conf(&thermal_protect_local_conf, VTSS_ISID_LOCAL);

    // Priorities temperatures configuration
    for (prio_index = THERMAL_PROTECT_PRIOS_MIN; prio_index <= THERMAL_PROTECT_PRIOS_MAX; prio_index++) {
      if (req->all_defaults ||
          (thermal_protect_local_conf.glbl_conf.prio_temperatures[prio_index] != THERMAL_PROTECT_TEMP_MAX)) {


        if (thermal_protect_local_conf.glbl_conf.prio_temperatures[prio_index] == THERMAL_PROTECT_TEMP_MAX) {
          VTSS_RC(vtss_icfg_printf(result, "no thermal-protect prio %d\n",
                                   prio_index));
        } else {
          VTSS_RC(vtss_icfg_printf(result, "thermal-protect prio %d temperature %d\n",
                                   prio_index,
                                   thermal_protect_local_conf.glbl_conf.prio_temperatures[prio_index]));
        }
      }
    }

    break;

  case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
    vtss_isid_t isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

    // Get configuration for the current switch
    thermal_protect_mgmt_get_switch_conf(&thermal_protect_local_conf, isid);

    T_I_PORT(iport, "req->all_defaults:%d", req->all_defaults);
    if (req->all_defaults ||
        (thermal_protect_local_conf.port_prio[iport] != 0)) {


      if (thermal_protect_local_conf.port_prio[iport] == 0) {
        VTSS_RC(vtss_icfg_printf(result, " no thermal-protect\n"));
      } else {
        VTSS_RC(vtss_icfg_printf(result, " thermal-protect prio %d\n", thermal_protect_local_conf.port_prio[iport]));
      }
    }
    break;
  }
  default:
    break;
  }
  return VTSS_RC_OK;
}


/* ICFG Initialization function */
vtss_rc thermal_protect_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_THERMAL_PROTECT_GLOBAL_CONF, thermal_protect_conf));
  return vtss_icfg_query_register(VTSS_ICFG_THERMAL_PROTECT_PORT_CONF, thermal_protect_conf);
}
#endif // VTSS_SW_OPTION_ICFG
#endif // #ifdef VTSS_SW_OPTION_THERMAL_PROTECT
/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
