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
#ifdef VTSS_SW_OPTION_LLDP_MED

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "lldp.h"
#include "lldp_api.h"
#include "lldp_os.h"
#include "msg_api.h"
#include "misc_api.h" // for uport2iport
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif
#include "mgmt_api.h"
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

#include "lldpmed_rx.h"

/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/

/**
 * \brief Help function that stores the LLDP configuration
 *
 * \param lldp_conf [IN] Pointer to the new LLDP configuration.
 * \param isid [IN] The internal switch id for which lldp_conf applies to.
 * \return None.
 **/
static void update_new_configuration(lldp_struc_0_t *lldp_conf, vtss_isid_t isid)
{
  if (lldp_mgmt_set_config(lldp_conf, VTSS_ISID_LOCAL) != VTSS_RC_OK) {
    T_E("Could not set new configuration for isid:%d", isid);
  }
}


// See lldpmed_icli_functions.h
void lldpmed_icli_civic_addr(i32 session_id, BOOL has_country, BOOL has_state, BOOL has_county, BOOL has_city, BOOL has_district, BOOL has_block, BOOL has_street, BOOL has_leading_street_direction, BOOL has_trailing_street_suffix, BOOL has_str_suf, BOOL has_house_no, BOOL has_house_no_suffix, BOOL has_landmark, BOOL has_additional_info, BOOL has_name, BOOL has_zip_code, BOOL has_building, BOOL has_apartment, BOOL has_floor, BOOL has_room_number, BOOL has_place_type, BOOL has_postal_com_name, BOOL has_p_o_box, BOOL has_additional_code, char *v_string250)
{
  lldp_struc_0_t      lldp_conf;

  if (v_string250 == NULL) {
    T_E("Sting is NULL - Should never happen");
    return;
  }

  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  if (has_country) {
    strcpy(lldp_conf.location_info.ca_country_code, v_string250);
  }

  if (has_state) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A1, v_string250);
  }

  if (has_county) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A2, v_string250);
  }

  if (has_city) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A3, v_string250);
  }

  if (has_district) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A4, v_string250);
  }

  if (has_block) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A5, v_string250);
  }

  if (has_street) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_A6, v_string250);
  }

  if (has_leading_street_direction) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_PRD, v_string250);
  }

  if (has_trailing_street_suffix) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_POD, v_string250);
  }

  if (has_str_suf) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_STS, v_string250);
  }

  if (has_house_no_suffix) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_HNS, v_string250);
  }

  if (has_house_no) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_HNO, v_string250);
  }

  if (has_landmark) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_LMK, v_string250);
  }

  if (has_additional_info) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_LOC, v_string250);
  }

  if (has_name) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_NAM, v_string250);
  }

  T_N("has_zip_code:%d - %s", has_zip_code, v_string250);
  if (has_zip_code) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_ZIP, v_string250);
  }

  if (has_building) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_BUILD, v_string250);
  }

  if (has_apartment) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_UNIT, v_string250);
  }

  if (has_floor) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_FLR, v_string250);
  }

  if (has_room_number) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_ROOM, v_string250);
  }

  if (has_place_type) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_PLACE, v_string250);
  }

  if (has_postal_com_name) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_PCN, v_string250);
  }

  if (has_p_o_box) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_POBOX, v_string250);
  }

  if (has_additional_code) {
    lldpmed_update_civic_info(&lldp_conf.location_info.civic, LLDPMED_CATYPE_ADD_CODE, v_string250);
  }

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}


// See lldpmed_icli_functions.h
void lldpmed_icli_elin_addr(char *elin_string)
{
  lldp_struc_0_t      lldp_conf;

  if (elin_string == NULL) {
    T_E("elin_string is NULL. Shall never happen");
    return;
  }

  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL); // Get current configuration

  strcpy(lldp_conf.location_info.ecs, elin_string); // Update the elin parameter.

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}


// Help function for printing neighbor inventory list
// In : session_id - Session_Id for ICLI_PRINTF
//      entry      - The information from the remote neighbor
static void icli_cmd_lldpmed_print_inventory(i32 session_id, lldp_remote_entry_t *entry)
{
  lldp_8_t inventory_str[MAX_LLDPMED_INVENTORY_LENGTH];


  if (entry->lldpmed_hw_rev_length            > 0 ||
      entry->lldpmed_firm_rev_length          > 0 ||
      entry->lldpmed_sw_rev_length            > 0 ||
      entry->lldpmed_serial_no_length         > 0 ||
      entry->lldpmed_manufacturer_name_length > 0 ||
      entry->lldpmed_model_name_length        > 0 ||
      entry->lldpmed_assert_id_length         > 0) {
    ICLI_PRINTF("\nInventory \n");

    strncpy(inventory_str, entry->lldpmed_hw_rev, entry->lldpmed_hw_rev_length);
    T_DG(TRACE_GRP_RX, "entry->lldpmed_hw_rev_length = %d", entry->lldpmed_hw_rev_length);
    inventory_str[entry->lldpmed_hw_rev_length] = '\0';// Clear the string since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Hardware Revision", inventory_str);


    strncpy(inventory_str, entry->lldpmed_firm_rev, entry->lldpmed_firm_rev_length);
    T_DG(TRACE_GRP_RX, "entry->lldpmed_hw_rev_length = %d", entry->lldpmed_firm_rev_length);
    inventory_str[entry->lldpmed_firm_rev_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Firmware Revision", inventory_str);

    strncpy(inventory_str, entry->lldpmed_sw_rev, entry->lldpmed_sw_rev_length);
    inventory_str[entry->lldpmed_sw_rev_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Software Revision", inventory_str);

    strncpy(inventory_str, entry->lldpmed_serial_no, entry->lldpmed_serial_no_length);
    inventory_str[entry->lldpmed_serial_no_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Serial Number", inventory_str);

    strncpy(inventory_str, entry->lldpmed_manufacturer_name, entry->lldpmed_manufacturer_name_length);
    inventory_str[entry->lldpmed_manufacturer_name_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Manufacturer Name", inventory_str);

    strncpy(inventory_str, entry->lldpmed_model_name, entry->lldpmed_model_name_length);
    inventory_str[entry->lldpmed_model_name_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Model Name", inventory_str);

    strncpy(inventory_str, entry->lldpmed_assert_id, entry->lldpmed_assert_id_length);
    inventory_str[entry->lldpmed_assert_id_length] = '\0'; // Add NULL pointer since data from the LLDP entry does contain NULL pointer.
    ICLI_PRINTF("%-20s: %s \n", "Assert ID", inventory_str);
  }
}

// Help function for printing the LLDP-MED neighbor infomation in an entry.
//
// In : session_id - Session_Id for ICLI_PRINTF
//      isid - Switch id for the switch from which the information should be shown.
//      has_interface - TRUE if the user has specified a specific interface to show
//      list - list containing information about which ports to show the information for.
static void icli_cmd_lldpmed_print_info(i32 session_id, vtss_isid_t isid, BOOL has_interface, icli_stack_port_range_t *list)
{
  lldp_8_t   buf[1000];
  lldp_u8_t  p_index;
  port_iter_t    pit;
  u32 i;
  lldp_remote_entry_t *table = NULL, *entry = NULL;
  BOOL lldpmed_no_entry_found = TRUE;

  if (!msg_switch_exists(isid)) {
    T_E("Isid:%d not found", isid); // Should never happen
    return;
  }

  // Loop through all front ports
  if (icli_port_iter_init(&pit, isid, PORT_ITER_SORT_ORDER_IPORT | PORT_ITER_FLAGS_FRONT) == VTSS_OK) {

    lldp_mgmt_get_lock();
    table = lldp_mgmt_get_entries(isid); // Get the LLDP entries for the switch in question.
    T_IG(TRACE_GRP_CLI, "has_interface:%d", has_interface);

    while (icli_port_iter_getnext(&pit, list)) {
      for (i = 0, entry = table; i < lldp_remote_get_max_entries(); i++, entry++) {
        if (entry->in_use == 0 || !entry->lldpmed_info_vld || (entry->receive_port != pit.iport)) {
          continue;
        }
        lldpmed_no_entry_found = FALSE;

        T_IG_PORT(TRACE_GRP_CLI, entry->receive_port, "size = %zu, lldpmed_info_vld = %d", sizeof(buf), entry->lldpmed_info_vld);

        // This function takes an iport and prints a uport.
        if (lldp_remote_receive_port_to_string(entry->receive_port, buf, isid)) {
          T_DG_PORT(TRACE_GRP_CLI, entry->receive_port, "Part of LAG");
        }


        if (entry->lldpmed_info_vld) {
          ICLI_PRINTF("%-20s: %s\n", "Local port", buf);


          // Device type / capabilities
          if (entry->lldpmed_capabilities_vld) {
            lldpmed_device_type2str(entry, buf);
            ICLI_PRINTF("%-20s: %s \n", "Device Type", buf);

            lldpmed_capabilities2str(entry, buf);
            ICLI_PRINTF("%-20s: %s \n", "Capabilities", buf);
          }

          // Loop through policies
          for (p_index = 0; p_index < MAX_LLDPMED_POLICY_APPLICATIONS_CNT; p_index ++) {
            // make sure that policy exist.
            T_NG(TRACE_GRP_CLI, "Policy valid = %d, p_index = %d", entry->lldpmed_policy_vld[p_index], p_index);
            if (entry->lldpmed_policy_vld[p_index] == LLDP_FALSE) {
              T_NG(TRACE_GRP_CLI, "Continue");
              continue;
            }

            // Policies
            lldpmed_policy_appl_type2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("\n%-20s: %s \n", "Application Type", buf);

            lldpmed_policy_flag_type2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("%-20s: %s \n", "Policy", buf);

            lldpmed_policy_tag2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("%-20s: %s \n", "Tag", buf);

            lldpmed_policy_vlan_id2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("%-20s: %s \n", "VLAN ID", buf);

            lldpmed_policy_prio2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("%-20s: %s \n", "Priority", buf);

            lldpmed_policy_dscp2str(entry->lldpmed_policy[p_index], buf);
            ICLI_PRINTF("%-20s: %s \n", "DSCP", buf);
          }


          for (p_index = 0; p_index < MAX_LLDPMED_LOCATION_CNT; p_index ++) {
            if (entry->lldpmed_location_vld[p_index] == 1) {
              lldpmed_location2str(entry, buf, p_index);
              ICLI_PRINTF("%-20s: %s \n", "Location", buf);
            }
          }

          icli_cmd_lldpmed_print_inventory(session_id, entry);

          ICLI_PRINTF("\n");
        }
      }
    }
    lldp_mgmt_get_unlock();

    T_DG(TRACE_GRP_CLI, "lldpmed_no_entry_found:%d", lldpmed_no_entry_found);
    if (lldpmed_no_entry_found) {
      ICLI_PRINTF("No LLDP-MED entries found\n");
    }
  }
}

// See lldpmed_icli_functions.h
void lldpmed_icli_show_remote_device(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;

  // Loop all the switches in question.
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, list)) {
      icli_cmd_lldpmed_print_info(session_id, sit.isid, has_interface, list);
    }
  }
}


// Help function for printing the policies list.
// In : Session_Id - session_id for ICLI_PRINTF
//      conf       - Current configuration
//      policy_index - Index for the policy to print.
static void lldpmed_icli_print_policies(i32 session_id, const lldp_struc_0_t lldp_conf, u32 policy_index)
{
  lldp_8_t application_type_str[200];

  if (lldp_conf.policies_table[policy_index].in_use) {
    lldpmed_appl_type2str(lldp_conf.policies_table[policy_index].application_type, &application_type_str[0]);
    ICLI_PRINTF("%-10d %-25s %-8s %-8d %-12d %-8d \n",
                policy_index,
                application_type_str,
                lldp_conf.policies_table[policy_index].tagged_flag ? "Tagged" : "Untagged",
                lldp_conf.policies_table[policy_index].vlan_id,
                lldp_conf.policies_table[policy_index].l2_priority,
                lldp_conf.policies_table[policy_index].dscp_value );
  }
}



// See lldpmed_icli_functions.h
void lldpmed_icli_show_polices(i32 session_id, icli_unsigned_range_t *policies_list)
{
  lldp_struc_0_t      lldp_conf;
  lldp_16_t policy_index;
  u32 i, idx;
  BOOL at_least_one_policy_defined = FALSE;
  // get current configuration
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  // Find out if any policy is defined.
  for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
    if (lldp_conf.policies_table[policy_index].in_use) {
      at_least_one_policy_defined = TRUE;
      break;
    }
  }

  // Print out "header"
  if (at_least_one_policy_defined) {
    ICLI_PRINTF("%-10s %-25s %-8s %-8s %-12s %-8s \n", "Policy Id", "Application Type", "Tag", "Vlan ID", "L2 Priority", "DSCP");
  } else {
    ICLI_PRINTF("No policies defined\n");
    return;
  }

  // Print all polices that are currently in use.
  if (policies_list != NULL) {
    // User want some specific policies
    T_IG(TRACE_GRP_CLI, "cnt:%u", policies_list->cnt);
    for ( i = 0; i < policies_list->cnt; i++ ) {
      T_IG(TRACE_GRP_CLI, "(%u, %u) ", policies_list->range[i].min, policies_list->range[i].max);
      for ( idx = policies_list->range[i].min; idx <= policies_list->range[i].max; idx++ ) {
        lldpmed_icli_print_policies(session_id, lldp_conf, idx);
      }
    }
  } else {
    // User want all policies
    for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
      lldpmed_icli_print_policies(session_id, lldp_conf, policy_index);
    }
  }
}


// See lldpmed_icli_functions.h
void lldpmed_icli_latitude(i32 session_id, BOOL north, BOOL south, char *degree)
{
  lldp_struc_0_t      lldp_conf;
  long value = 0;
  // get current configuration
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  // convert floating point "string value" to long
  T_IG(TRACE_GRP_CLI, "Degree:%s", degree);
  if (mgmt_str_float2long(degree, &value, LLDPMED_LATITUDE_VALUE_MIN, LLDPMED_LATITUDE_VALUE_MAX, TUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Degree \"%s\" is not valid. Must be in the range 0.0000 to 90.0000\n", degree);
    return;
  }
  lldp_conf.location_info.latitude = value;


  if (north) {
    lldp_conf.location_info.latitude_dir = NORTH;
  }

  if (south) {
    lldp_conf.location_info.latitude_dir = SOUTH;
  }


  // get current configuration
  if (lldp_mgmt_set_config(&lldp_conf, VTSS_ISID_LOCAL) != VTSS_RC_OK) {
    T_E("Could not set conf");
  }
}

// See lldpmed_icli_functions.h
void lldpmed_icli_longitude(i32 session_id, BOOL east, BOOL west, char *degree)
{
  lldp_struc_0_t      lldp_conf;
  long value = 0;
  // get current configuration
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);


  // convert floating point "string value" to long
  T_IG(TRACE_GRP_CLI, "Degree:%s", degree);
  if (mgmt_str_float2long(degree, &value, LLDPMED_LONGITUDE_VALUE_MIN, LLDPMED_LONGITUDE_VALUE_MAX, TUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Degree \"%s\" is not valid. Must be in the range 0.0000 to 180.0000\n", degree);
    return;
  }
  lldp_conf.location_info.longitude = value;


  if (east) {
    lldp_conf.location_info.longitude_dir = EAST;
  }

  if (west) {
    lldp_conf.location_info.longitude_dir = WEST;
  }

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}


// See lldpmed_icli_functions.h
void lldpmed_icli_altitude(i32 session_id, BOOL meters, BOOL floors, char *value_str)
{
  lldp_struc_0_t      lldp_conf;
  long value = 0;
  // get current configuration
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);


  // convert floating point "string value" to long
  if (mgmt_str_float2long(value_str, &value, LLDPMED_ALTITUDE_VALUE_MIN, LLDPMED_ALTITUDE_VALUE_MAX, TUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Altitude \"%s\" is not valid. Must be in the range -32767.0000 to 32767.0000\n", value_str);
    return;
  }

  lldp_conf.location_info.altitude = value;


  if (meters) {
    lldp_conf.location_info.altitude_type = METERS;
  }

  if (floors) {
    lldp_conf.location_info.altitude_type = FLOOR;
  }

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}

// See lldpmed_icli_functions.h
void lldpmed_icli_datum(BOOL has_wgs84, BOOL has_nad83_navd88, BOOL has_nad83_mllw, BOOL no)
{
  // get current configuration
  lldp_struc_0_t      lldp_conf;
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  if (no) {
    lldp_conf.location_info.datum = LLDPMED_DATUM_DEFAULT;
  } else if (has_wgs84) {
    lldp_conf.location_info.datum = WGS84;
  } else if (has_nad83_navd88) {
    lldp_conf.location_info.datum = NAD83_NAVD88;
  } else if (has_nad83_mllw) {
    lldp_conf.location_info.datum = NAD83_MLLW;
  }

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}

// See lldpmed_icli_functions.h
void lldpmed_icli_fast_start(u32 value, BOOL no)
{
  lldp_struc_0_t      lldp_conf;
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);

  if (no) {
    lldp_conf.medFastStartRepeatCount = FAST_START_REPEAT_COUNT_DEFAULT;
  } else {
    lldp_conf.medFastStartRepeatCount = value;
  }

  update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
}


// See lldpmed_icli_functions.h
void lldpmed_icli_assign_policy(i32 session_id, icli_stack_port_range_t *port_list, icli_range_t *policies_list, BOOL no)
{
  vtss_isid_t isid;
  // Just making sure that we don't access NULL pointer.
  if (port_list != NULL) {
    isid = topo_usid2isid(port_list->switch_range[0].usid);
  } else {
    isid = VTSS_ISID_LOCAL;
  }

  port_iter_t pit;
  lldp_struc_0_t      lldp_conf;

  // Get current configuration
  lldp_mgmt_get_config(&lldp_conf, isid);

  // Loop through ports.
  if (icli_port_iter_init(&pit, isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK) == VTSS_RC_OK) {


    while (icli_port_iter_getnext(&pit, port_list)) {
      u32 p_index;
      i32 range_index;


      // If user doesn't select some specific policies then remove them all (This only should only happen for the "no" command
      if (policies_list == NULL) {
        if (!no) {
          T_E("policies list is NULL. Should only happen for the no command");
        }
        // Remove all policies from this interface port.
        for (range_index = 0; range_index < LLDPMED_POLICIES_CNT; range_index++) {
          lldp_conf.ports_policies[pit.iport][range_index] = FALSE;
        }
      } else {
        for (p_index = 0; p_index < policies_list->u.sr.cnt; p_index++) {

          for (range_index = policies_list->u.sr.range[p_index].min; range_index <= policies_list->u.sr.range[p_index].max; range_index++) {

            if (no) {
              lldp_conf.ports_policies[pit.iport][range_index] = FALSE; // Remove the policy from this port
            } else {
              // Make sure we don't get out of bounce
              if (range_index > LLDPMED_POLICY_MAX) {
                ICLI_PRINTF("%% Ignoring invalid policy:%d. Valid range is %lu-%lu\n", range_index, LLDPMED_POLICY_MIN, LLDPMED_POLICY_MAX);
                continue;
              }

              if ((lldp_conf.policies_table[range_index].in_use == 0)) {
                ICLI_PRINTF("Ignoring policy %d for port %lu because no such policy is defined \n", range_index, pit.uport);
              } else {
                lldp_conf.ports_policies[pit.iport][range_index] = TRUE;
              }
            }
          }
        }
      }
    }
  }

  update_new_configuration(&lldp_conf, isid);
}
// See lldpmed_icli_functions.h
void lldpmed_icli_media_vlan_policy(i32 session_id, u32 policy_index, BOOL has_voice, BOOL has_voice_signaling, BOOL has_guest_voice_signaling,
                                    BOOL has_guest_voice, BOOL has_softphone_voice, BOOL has_video_conferencing, BOOL has_streaming_video,
                                    BOOL has_video_signaling, BOOL has_tagged, BOOL has_untagged, u32 v_vlan_id, u32 v_0_to_7, u32 v_0_to_63,
                                    BOOL no)
{

  lldp_struc_0_t      lldp_conf;
  lldp_mgmt_get_config(&lldp_conf, VTSS_ISID_LOCAL);
  BOOL update_conf = TRUE;

  // Loop through all poicies to find a free policy entry to add the new policy.
  if (no) {
    lldp_conf.policies_table[policy_index].in_use = FALSE;
  } else {
    lldp_conf.policies_table[policy_index].in_use = TRUE;

    if (has_voice) {
      lldp_conf.policies_table[policy_index].application_type = VOICE;
    } else if (has_voice_signaling) {
      lldp_conf.policies_table[policy_index].application_type = VOICE_SIGNALING;
    } else if (has_guest_voice) {
      lldp_conf.policies_table[policy_index].application_type = GUEST_VOICE;
    } else if (has_guest_voice_signaling) {
      lldp_conf.policies_table[policy_index].application_type = GUEST_VOICE_SIGNALING;
    } else if (has_softphone_voice) {
      lldp_conf.policies_table[policy_index].application_type = SOFTPHONE_VOICE;
    } else if (has_video_conferencing) {
      lldp_conf.policies_table[policy_index].application_type = VIDEO_CONFERENCING;
    } else if (has_streaming_video) {
      lldp_conf.policies_table[policy_index].application_type = STREAMING_VIDEO;
    } else if (has_video_signaling) {
      lldp_conf.policies_table[policy_index].application_type = VIDEO_SIGNALING;
    }

    lldp_conf.policies_table[policy_index].tagged_flag = has_tagged;

    if (has_tagged) {
      if (v_vlan_id == 0) {
        ICLI_PRINTF("VLAN id can't be 0 when tagged.\n");
        update_conf = FALSE;
      }
    }
    T_IG(TRACE_GRP_CLI, "vlan_id:%u has_tagged:%d", v_vlan_id, has_tagged);

    lldp_conf.policies_table[policy_index].vlan_id = v_vlan_id;
    lldp_conf.policies_table[policy_index].l2_priority = v_0_to_7;
    lldp_conf.policies_table[policy_index].dscp_value = v_0_to_63;
  }

  if (update_conf) {
    update_new_configuration(&lldp_conf, VTSS_ISID_LOCAL);
  }
}


// See lldpmed_icli_functions.h
void lldpmed_icli_transmit_tlv(icli_stack_port_range_t *list, BOOL has_capabilities, BOOL has_location, BOOL has_network_policy, BOOL no)
{
  lldp_struc_0_t      lldp_conf;
  port_iter_t pit;
  vtss_isid_t isid;


  // Just making sure that we don't access NULL pointer.
  if (list != NULL) {
    isid = topo_usid2isid(list->switch_range[0].usid);
  } else {
    isid = VTSS_ISID_LOCAL;
  }

  // get current configuration
  lldp_mgmt_get_config(&lldp_conf, isid);

  // Loop through ports.
  if (icli_port_iter_init(&pit, isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, list)) {
      T_IG_PORT(TRACE_GRP_CLI, pit.iport, "no:%d, has_capabilities:%d, tlv:0x%X", no, has_capabilities, lldp_conf.lldpmed_optional_tlv[pit.iport]);
      if (has_capabilities) {
        if (no) {
          // Clear the capabilities bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] &= ~OPTIONAL_TLV_CAPABILITIES_BIT;
        } else {
          // Set the capabilities bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] |= OPTIONAL_TLV_CAPABILITIES_BIT;
        }
      }

      if (has_network_policy) {
        if (no) {
          // Clear the network-policy bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] &= ~OPTIONAL_TLV_POLICY_BIT;
        } else {
          // Set the network-policy bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] |= OPTIONAL_TLV_POLICY_BIT;
        }
      }

      if (has_location) {
        if (no) {
          // Clear the location bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] &= ~OPTIONAL_TLV_LOCATION_BIT;
        } else {
          // Set the location bit.
          lldp_conf.lldpmed_optional_tlv[pit.iport] |= OPTIONAL_TLV_LOCATION_BIT;
        }
      }
      T_IG_PORT(TRACE_GRP_CLI, pit.iport, "tlv:0x%X", lldp_conf.lldpmed_optional_tlv[pit.iport]);
    }
  }

  update_new_configuration(&lldp_conf, isid);
}



//
// ICFG (Show running)
//
#ifdef VTSS_SW_OPTION_ICFG

// help function for getting Ca type as printable keyword
//
// In : ca_type - CA TYPE as integer
//
// In/Out: key_word - Pointer to the string.
//
static void lldpmed_catype2keyword(lldpmed_catype_t ca_type, char *key_word)
{
  // Table in ANNEX B, TIA1057
  strcpy(key_word, "");
  switch (ca_type) {
  case LLDPMED_CATYPE_A1:
    strcat(key_word, "National subdivison");
    break;
  case LLDPMED_CATYPE_A2:
    strcat(key_word, "county");
    break;
  case LLDPMED_CATYPE_A3:
    strcat(key_word, "city");
    break;
  case LLDPMED_CATYPE_A4:
    strcat(key_word, "district");
    break;
  case LLDPMED_CATYPE_A5:
    strcat(key_word, "block");
    break;
  case LLDPMED_CATYPE_A6:
    strcat(key_word, "street");
    break;
  case LLDPMED_CATYPE_PRD:
    strcat(key_word, "leading-street-direction");
    break;
  case LLDPMED_CATYPE_POD:
    strcat(key_word, "trailing-street-suffix");
    break;
  case LLDPMED_CATYPE_STS:
    strcat(key_word, "street-suffix");
    break;
  case LLDPMED_CATYPE_HNO:
    strcat(key_word, "house-no");
    break;
  case LLDPMED_CATYPE_HNS:
    strcat(key_word, "house-no-suffix");
    break;
  case LLDPMED_CATYPE_LMK:
    strcat(key_word, "landmark");
    break;
  case LLDPMED_CATYPE_LOC:
    strcat(key_word, "additional-info");
    break;
  case LLDPMED_CATYPE_NAM:
    strcat(key_word, "name");
    break;
  case LLDPMED_CATYPE_ZIP:
    strcat(key_word, "zip-code");
    break;
  case LLDPMED_CATYPE_BUILD:
    strcat(key_word, "building");
    break;
  case LLDPMED_CATYPE_UNIT:
    strcat(key_word, "apartment");
    break;
  case LLDPMED_CATYPE_FLR:
    strcat(key_word, "floor");
    break;
  case LLDPMED_CATYPE_ROOM:
    strcat(key_word, "room-number");
    break;
  case LLDPMED_CATYPE_PLACE:
    strcat(key_word, "place-type");
    break;
  case LLDPMED_CATYPE_PCN:
    strcat(key_word, "postal-community-name");
    break;
  case LLDPMED_CATYPE_POBOX:
    strcat(key_word, "p-o-box");
    break;
  case LLDPMED_CATYPE_ADD_CODE:
    strcat(key_word, "additional-code");
    break;
  default:
    break;
  }
}


// Help function for printing the civic address
//
// In : all_defaults - TRUE if user want include printing of default values.
//      ca_type      - CA type to print
//      lldp_conf    - Pointer to LLDP configuration.
// In/Out: result - Pointer for ICFG
static vtss_rc lldpmed_icfg_print_civic(BOOL all_defaults, lldpmed_catype_t ca_type, vtss_icfg_query_result_t *result, lldp_struc_0_t *lldp_conf)
{
  lldp_8_t key_word_str[CIVIC_CA_VALUE_LEN_MAX];
  BOOL civic_empty;

  lldpmed_catype2keyword(ca_type, &key_word_str[0]); // Converting ca type to a printable string
  u16 ptr = lldp_conf->location_info.civic.civic_str_ptr_array[lldpmed_catype2index(ca_type)];

  civic_empty = (strlen(&lldp_conf->location_info.civic.ca_value[ptr]) == 0);

  if (all_defaults ||
      (!civic_empty)) {

    VTSS_RC(vtss_icfg_printf(result, "%slldp med location-tlv civic-addr %s %s\n",
                             civic_empty ? "no " : "",
                             key_word_str,
                             civic_empty ? "" : &lldp_conf->location_info.civic.ca_value[ptr] ));
  }
  return VTSS_RC_OK;
}


// Help function for print global configuration.
// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      iport - Port in question
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static vtss_rc lldpmed_icfg_print_global(lldp_struc_0_t *lldp_conf, vtss_port_no_t iport, BOOL all_defaults, vtss_icfg_query_result_t *result)
{

  // Altitude_
  if (all_defaults ||
      (lldp_conf->location_info.altitude_type != LLDPMED_ALTITUDE_TYPE_DEFAULT) ||
      (lldp_conf->location_info.altitude != LLDPMED_ALTITUDE_DEFAULT)) {

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv altitude %s %d\n",
                             lldp_conf->location_info.altitude_type == METERS ? "meters" : "floors",
                             lldp_conf->location_info.altitude));
  }

  // Latitude_
  if (all_defaults ||
      (lldp_conf->location_info.latitude_dir != LLDPMED_LATITUDE_DIR_DEFAULT) ||
      (lldp_conf->location_info.latitude != LLDPMED_LATITUDE_DEFAULT)) {

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv latitude %s %d\n",
                             lldp_conf->location_info.latitude_dir == SOUTH ? "south" : "north",
                             lldp_conf->location_info.latitude));
  }


  // Longitude_
  if (all_defaults ||
      (lldp_conf->location_info.longitude_dir != LLDPMED_LONGITUDE_DIR_DEFAULT) ||
      (lldp_conf->location_info.longitude != LLDPMED_LONGITUDE_DEFAULT)) {

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv longitude %s %d\n",
                             lldp_conf->location_info.longitude_dir == EAST ? "east" : "west",
                             lldp_conf->location_info.longitude));
  }


  // Datum
  if (all_defaults ||
      lldp_conf->location_info.datum != LLDPMED_DATUM_DEFAULT) {

    VTSS_RC(vtss_icfg_printf(result, "%slldp med datum %s\n",
                             lldp_conf->location_info.datum == LLDPMED_DATUM_DEFAULT ? "no " : "",
                             lldp_conf->location_info.datum == WGS84 ? "wgs84" :
                             lldp_conf->location_info.datum == NAD83_NAVD88 ? "nad83_navd88" :
                             "nad83_mllw"));
  }

  // Fast start
  if (all_defaults ||
      lldp_conf->medFastStartRepeatCount != FAST_START_REPEAT_COUNT_DEFAULT) {

    if (lldp_conf->medFastStartRepeatCount == FAST_START_REPEAT_COUNT_DEFAULT) {
      VTSS_RC(vtss_icfg_printf(result, "no lldp med fast\n"));
    } else {
      VTSS_RC(vtss_icfg_printf(result, "lldp med fast %lu\n", lldp_conf->medFastStartRepeatCount));
    }
  }

  // Civic address
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A1, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A2, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A3, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A4, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A5, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A6, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PRD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_POD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_STS, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_HNO, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_HNS, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_LMK, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_LOC, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_NAM, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ZIP, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_BUILD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_UNIT, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_FLR, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ROOM, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PLACE, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PCN, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_POBOX, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ADD_CODE, result, lldp_conf));


  // Elin
  BOOL is_ecs_non_default = strlen(lldp_conf->location_info.ecs) != 0; // Default value for ecs is empty string
  if (all_defaults ||
      is_ecs_non_default) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp med location-tlv elin-addr %s\n",
                             is_ecs_non_default ? "" : "no ",
                             lldp_conf->location_info.ecs));
  }

  //
  // Policies
  //
  const i32 POLICY_INDEX_NOT_FOUND = -1;
  // First print all no used policies (Default is "no policy", so only print if "print all default" is used)
  u32 policy_index;
  if (all_defaults) {
    BOOL print_command = TRUE;
    i32 start_policy = POLICY_INDEX_NOT_FOUND, end_policy = POLICY_INDEX_NOT_FOUND;
    for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
      if (lldp_conf->policies_table[policy_index].in_use == FALSE) {
        if (start_policy == POLICY_INDEX_NOT_FOUND) {
          start_policy = policy_index;
        }
      } else {
        start_policy = POLICY_INDEX_NOT_FOUND;
        end_policy = POLICY_INDEX_NOT_FOUND;
      }

      if (policy_index != LLDPMED_POLICY_MAX) { // Make sure that we don't get out of bounds
        if (lldp_conf->policies_table[policy_index + 1].in_use) {
          end_policy = policy_index;
        }
      } else {
        end_policy = LLDPMED_POLICY_MAX;
      }


      if ((start_policy != POLICY_INDEX_NOT_FOUND) && (end_policy != POLICY_INDEX_NOT_FOUND)) {
        // Only print command the first time a
        if (print_command) {
          VTSS_RC(vtss_icfg_printf(result, "no lldp med media-vlan-policy" , policy_index));
          print_command = FALSE;
        } else {
          VTSS_RC(vtss_icfg_printf(result, ","));
        }
        if (start_policy == end_policy) {
          VTSS_RC(vtss_icfg_printf(result, "%d", start_policy));
        } else {
          VTSS_RC(vtss_icfg_printf(result, "%d-%d", start_policy, end_policy));
        }
      }
    } // End for

    if (!print_command) {
      VTSS_RC(vtss_icfg_printf(result, "\n"));
    }
  }

  // Print all policies defined
  for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
    if (lldp_conf->policies_table[policy_index].in_use == TRUE) {

      VTSS_RC(vtss_icfg_printf(result, "lldp med media-vlan-policy %d", policy_index));

      switch (lldp_conf->policies_table[policy_index].application_type) {
      case VOICE:
        VTSS_RC(vtss_icfg_printf(result, " voice"));
        break;
      case VOICE_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " voice-signaling"));
        break;
      case GUEST_VOICE:
        VTSS_RC(vtss_icfg_printf(result, " guest-voice"));
        break;
      case GUEST_VOICE_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " guest-voice-signaling"));
        break;
      case SOFTPHONE_VOICE:
        VTSS_RC(vtss_icfg_printf(result, " softphone-voice"));
        break;
      case VIDEO_CONFERENCING:
        VTSS_RC(vtss_icfg_printf(result, " video-conferencing"));
        break;
      case STREAMING_VIDEO:
        VTSS_RC(vtss_icfg_printf(result, " streaming-video"));
        break;

      case VIDEO_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " video-signaling"));
        break;
      }

      if (lldp_conf->policies_table[policy_index].tagged_flag) {
        VTSS_RC(vtss_icfg_printf(result, " tagged"));
        VTSS_RC(vtss_icfg_printf(result, " %d", lldp_conf->policies_table[policy_index].vlan_id));
      } else {
        VTSS_RC(vtss_icfg_printf(result, " untagged"));
      }



      VTSS_RC(vtss_icfg_printf(result, " l2-priority %d", lldp_conf->policies_table[policy_index].l2_priority));

      VTSS_RC(vtss_icfg_printf(result, " dscp %d\n", lldp_conf->policies_table[policy_index].dscp_value));
    }
  }

  return VTSS_RC_OK;
}

// Help function for determining if a port has any policies assigned.
// In : iport - port in question.
//      lldp_conf - LLDP configuration
// Return: TRUE if at least on policy is assigned to the port, else FALSE
static BOOL is_any_policies_assigned(vtss_port_no_t iport, const lldp_struc_0_t lldp_conf)
{
  u32 policy;

  for (policy = LLDPMED_POLICY_MIN; policy <= LLDPMED_POLICY_MAX; policy++) {
    if (lldp_conf.ports_policies[iport][policy]) {
      T_DG_PORT(TRACE_GRP_CLI, iport, "lldp_conf.ports_policies[iport][%d]:0x%X", lldp_conf.ports_policies[iport][policy], policy);
      return TRUE; // At least one policy set
    }
  }

  return FALSE;
}

// Function called by ICFG.
static vtss_rc lldpmed_icfg_global_conf(const vtss_icfg_query_request_t *req,
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
    VTSS_RC(lldpmed_icfg_print_global(&lldp_conf, 0, req->all_defaults, result));
    break;
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);
    T_D("Isid:%d, iport:%u, req->instance_id.port.usid:%d", isid, iport, req->instance_id.port.usid);

    if (msg_switch_exists(isid)) {
      // Get current configuration for this switch
      lldp_mgmt_get_config(&lldp_conf, isid);

      //
      // Optional TLVs
      //
      BOOL tlv_capabilities_disabled = (lldp_conf.lldpmed_optional_tlv[iport] & OPTIONAL_TLV_CAPABILITIES_BIT) == 0;
      BOOL tlv_location_disabled = (lldp_conf.lldpmed_optional_tlv[iport] & OPTIONAL_TLV_LOCATION_BIT) == 0;
      BOOL tlv_policy_disabled = (lldp_conf.lldpmed_optional_tlv[iport] & OPTIONAL_TLV_POLICY_BIT) == 0;

      BOOL at_least_one_tlv_is_disabled = tlv_capabilities_disabled || tlv_location_disabled || tlv_policy_disabled;
      BOOL at_least_one_tlv_is_enabled = !tlv_capabilities_disabled || !tlv_location_disabled || !tlv_policy_disabled;

      //In Order to make the "no" command work logically we act as the default value is "disabled" regardless of how it is in the lldp.c
      if (at_least_one_tlv_is_enabled) {
        VTSS_RC(vtss_icfg_printf(result, " lldp med location-tlv"));

        VTSS_RC(vtss_icfg_printf(result, "%s",  tlv_capabilities_disabled ? "" : " capabilities"));
        VTSS_RC(vtss_icfg_printf(result, "%s",  tlv_location_disabled ? "" : " location"));
        VTSS_RC(vtss_icfg_printf(result, "%s",  tlv_policy_disabled ? "" : " network-policy"));

        VTSS_RC(vtss_icfg_printf(result, "\n"));
      }

      if (at_least_one_tlv_is_disabled) {

        if (req->all_defaults) {

          VTSS_RC(vtss_icfg_printf(result, " no lldp med location-tlv"));

          VTSS_RC(vtss_icfg_printf(result, " %s",  tlv_capabilities_disabled ? "capabilities" : ""));
          VTSS_RC(vtss_icfg_printf(result, " %s",  tlv_location_disabled ? "location" : ""));
          VTSS_RC(vtss_icfg_printf(result, " %s",  tlv_policy_disabled ? "network-policy" : ""));

          VTSS_RC(vtss_icfg_printf(result, "\n"));
        }
      }


      // Policies assigned to the port.
      i8 buf[150];
      if (req->all_defaults ||
          is_any_policies_assigned(iport, lldp_conf)) {

        if (is_any_policies_assigned(iport, lldp_conf)) {
          // Convert boolean list to printable string.
          (void) mgmt_non_portlist2txt(lldp_conf.ports_policies[iport], LLDPMED_POLICY_MIN, LLDPMED_POLICY_MAX, buf);
          VTSS_RC(vtss_icfg_printf(result, " lldp med media-vlan policy-list %s\n", buf));
        } else {
          VTSS_RC(vtss_icfg_printf(result, " no lldp med media-vlan policy-list\n"));
        }
      }
      break;

    default:
      //Not needed for LLDP
      break;
    }
  } // End msg_switch_exists
  return VTSS_RC_OK;
}

/* ICFG Initialization function */
vtss_rc lldpmed_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDPMED_GLOBAL_CONF, lldpmed_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDPMED_PORT_CONF, lldpmed_icfg_global_conf));
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG

#endif // #ifdef VTSS_SW_OPTION_LLDP_MED
/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
