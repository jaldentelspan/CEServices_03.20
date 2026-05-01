/*

 Vitesse API software.

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

#ifndef _VTSS_COMMON_H_
#define _VTSS_COMMON_H_

#include "vtss_api.h"

u32 vtss_cmn_pcp2qos(u32 pcp);
vtss_rc vtss_port_no_check(const vtss_port_no_t port_no);
vtss_rc vtss_cmn_restart_update(u32 value);
u32 vtss_cmn_restart_value_get(void);

#if defined(VTSS_FEATURE_PORT_CONTROL)  ||  defined(VTSS_FEATURE_OAM)
void vtss_cmn_counter_32_rebase(u32 new_base_value, vtss_chip_counter_t *counter);
void vtss_cmn_counter_32_update(u32 value, vtss_chip_counter_t *counter, BOOL clear);
void vtss_cmn_counter_40_rebase(u32 new_lsb, u32 new_msb, vtss_chip_counter_t *counter);
void vtss_cmn_counter_40_update(u32 lsb, u32 msb, vtss_chip_counter_t *counter, BOOL clear);
#endif

#if defined(VTSS_FEATURE_PORT_CONTROL)
vtss_port_no_t vtss_cmn_port2port_no(const vtss_debug_info_t *const info, u32 port);
vtss_rc vtss_cmn_port_clause_37_adv_get(u32 value, vtss_port_clause_37_adv_t *adv);
vtss_rc vtss_cmn_port_clause_37_adv_set(u32 *value, vtss_port_clause_37_adv_t *adv, BOOL aneg_enable);
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_LAYER2)
vtss_rc vtss_cmn_vlan_members_get(vtss_state_t *state, 
                                  const vtss_vid_t vid,
                                  BOOL member[VTSS_PORT_ARRAY_SIZE]);
vtss_rc vtss_cmn_vlan_members_set(const vtss_vid_t vid);
vtss_rc vtss_cmn_vlan_port_conf_set(const vtss_port_no_t port_no);
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
vtss_rc vtss_cmn_vlan_tx_tag_set(const vtss_vid_t         vid,
                                 const vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE]);
#endif /* VTSS_FEATURE_VLAN_TX_TAG */
vtss_rc vtss_cmn_vlan_update_all(void);
vtss_rc vtss_cmn_mstp_state_set(const vtss_port_no_t   port_no,
                                const vtss_msti_t      msti);
vtss_rc vtss_cmn_erps_vlan_member_set(const vtss_erpi_t erpi,
                                      const vtss_vid_t  vid);
vtss_rc vtss_cmn_erps_port_state_set(const vtss_erpi_t    erpi,
                                     const vtss_port_no_t port_no);
vtss_rc vtss_cmn_eps_port_set(const vtss_port_no_t port_w);
void vtss_debug_print_mac_entry(const vtss_debug_printf_t pr,
                                const char *name,
                                BOOL *header,
                                vtss_mac_table_entry_t *entry,
                                u32 pgid);
#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
u32 vtss_cmn_ip2u32(vtss_ip_addr_internal_t *ip, BOOL ipv6);
vtss_rc vtss_cmn_ipv4_mc_add(const vtss_vid_t  vid,
                             const vtss_ip_t   sip,
                             const vtss_ip_t   dip,
                             const BOOL        member[VTSS_PORT_ARRAY_SIZE]);
vtss_rc vtss_cmn_ipv4_mc_del(const vtss_vid_t  vid,
                             const vtss_ip_t   sip,
                             const vtss_ip_t   dip);
vtss_rc vtss_cmn_ipv6_mc_add(const vtss_vid_t  vid,
                             const vtss_ipv6_t sip,
                             const vtss_ipv6_t dip,
                             const BOOL        member[VTSS_PORT_ARRAY_SIZE]);
vtss_rc vtss_cmn_ipv6_mc_del(const vtss_vid_t  vid,
                             const vtss_ipv6_t sip,
                             const vtss_ipv6_t dip);
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_VCAP)
void vtss_cmn_res_init(vtss_res_t *res);
vtss_rc vtss_cmn_vcap_res_check(vtss_vcap_obj_t *obj, vtss_res_chg_t *chg);
vtss_rc vtss_cmn_res_check(vtss_res_t *res);

BOOL vtss_vcap_udp_tcp_rule(const vtss_vcap_u8_t *proto);
vtss_rc vtss_vcap_range_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                              u32 *range,
                              vtss_vcap_range_chk_type_t type,
                              u32 min,
                              u32 max);
vtss_rc vtss_vcap_range_free(vtss_vcap_range_chk_table_t *range_chk_table,
                             u32 range);
vtss_rc vtss_vcap_udp_tcp_range_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                                      u32 *range, 
                                      const vtss_vcap_udp_tcp_t *port, 
                                      BOOL sport);
vtss_rc vtss_vcap_vr_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                           u32 *range,
                           vtss_vcap_range_chk_type_t type,
                           vtss_vcap_vr_t *vr);
vtss_rc vtss_vcap_range_commit(vtss_vcap_range_chk_table_t *range_new);
u32 vtss_vcap_key_rule_count(vtss_vcap_key_size_t key_size);
char *vtss_vcap_id_txt(vtss_vcap_id_t id);
vtss_rc vtss_vcap_lookup(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id, 
                         vtss_vcap_data_t *data, vtss_vcap_idx_t *idx);
vtss_rc vtss_vcap_del(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id);
vtss_rc vtss_vcap_add(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id, 
                      vtss_vcap_id_t ins_id, vtss_vcap_data_t *data, BOOL dont_add);
vtss_rc vtss_vcap_get_next_id(vtss_vcap_obj_t *obj, int user1, int user2, 
                              vtss_vcap_id_t id, vtss_vcap_id_t *ins_id);
#if defined(VTSS_FEATURE_IS1)
void vtss_vcap_is1_init(vtss_vcap_data_t *data, vtss_is1_entry_t *entry);
#endif /* VTSS_FEATURE_IS1 */
#if defined(VTSS_FEATURE_IS2)
void vtss_vcap_is2_init(vtss_vcap_data_t *data, vtss_is2_entry_t *entry);
#endif /* VTSS_FEATURE_IS2 */
#if defined(VTSS_FEATURE_ES0)
void vtss_vcap_es0_init(vtss_vcap_data_t *data, vtss_es0_entry_t *entry);
void vtss_cmn_es0_action_get(vtss_es0_data_t *es0);
#endif /* VTSS_FEATURE_ES0 */
const char *vtss_vcap_key_size_txt(vtss_vcap_key_size_t key_size);

#endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_FEATURE_ACL)
vtss_rc vtss_cmn_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace);
vtss_rc vtss_cmn_ace_del(const vtss_ace_id_t ace_id);
vtss_rc vtss_cmn_ace_counter_get(const vtss_ace_id_t ace_id, vtss_ace_counter_t *const counter);
vtss_rc vtss_cmn_ace_counter_clear(const vtss_ace_id_t ace_id);
#endif /* VTSS_FEATURE_ACL */

#if defined(VTSS_FEATURE_QOS)
vtss_rc vtss_cmn_qos_port_conf_set(const vtss_port_no_t port_no);
vtss_rc vtss_cmn_qos_weight2cost(const vtss_pct_t * weight, u8 * cost, size_t num, u8 bit_width);
#if defined(VTSS_FEATURE_QCL_V2)
vtss_rc vtss_cmn_qce_add(const vtss_qcl_id_t qcl_id, const vtss_qce_id_t qce_id, const vtss_qce_t *const qce);
vtss_rc vtss_cmn_qce_del(const vtss_qcl_id_t qcl_id, const vtss_qce_id_t qce_id);
#endif /* VTSS_FEATURE_QCL_V2 */
#endif /* VTSS_FEATURE_QOS */
#if defined(VTSS_FEATURE_VCL)
vtss_rc vtss_cmn_vce_add(const vtss_vce_id_t vce_id, const vtss_vce_t *const vce);
vtss_rc vtss_cmn_vce_del(const vtss_vce_id_t vce_id);
#endif /* VTSS_FEATURE_VCL */
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
vtss_rc vtss_cmn_vlan_trans_group_add(const u16 group_id, const vtss_vid_t vid, const vtss_vid_t trans_vid);
vtss_rc vtss_cmn_vlan_trans_group_del(const u16 group_id, const vtss_vid_t vid);
vtss_rc vtss_cmn_vlan_trans_group_get(vtss_vlan_trans_grp2vlan_conf_t *conf, BOOL next);
vtss_rc vtss_cmn_vlan_trans_port_conf_set(const vtss_vlan_trans_port2grp_conf_t *conf);
vtss_rc vtss_cmn_vlan_trans_port_conf_get(vtss_vlan_trans_port2grp_conf_t *conf, BOOL next);
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */

#if defined(VTSS_SDX_CNT)
vtss_sdx_entry_t *vtss_cmn_sdx_alloc(vtss_port_no_t port_no, BOOL isdx);
void vtss_cmn_sdx_free(vtss_sdx_entry_t *sdx, BOOL isdx);
vtss_sdx_entry_t *vtss_cmn_ece_sdx_alloc(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx);
vtss_rc vtss_cmn_ece_sdx_free(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx);
#endif /* VTSS_SDX_CNT */

#if defined(VTSS_FEATURE_EVC)
BOOL vtss_cmn_ece_es0_needed(vtss_ece_entry_t *ece);
vtss_ece_dir_t vtss_cmn_ece_dir_get(vtss_ece_entry_t *ece);
#if defined(VTSS_ARCH_SERVAL)
vtss_ece_rule_t vtss_cmn_ece_rule_get(vtss_ece_entry_t *ece);
#endif /* VTSS_ARCH_SERVAL */
vtss_vcap_bit_t vtss_cmn_ece_bit_get(vtss_ece_entry_t *ece, u32 mask_vld, u32 mask_1);
vtss_rc vtss_cmn_evc_add(const vtss_evc_id_t evc_id, const vtss_evc_conf_t *const conf);
vtss_rc vtss_cmn_evc_del(const vtss_evc_id_t evc_id);
vtss_rc vtss_cmn_ece_add(const vtss_ece_id_t ece_id, const vtss_ece_t *const ece);
vtss_rc vtss_cmn_ece_del(const vtss_ece_id_t ece_id);
#if defined(VTSS_SDX_CNT)
vtss_rc vtss_cmn_ece_counters_get(const vtss_ece_id_t  ece_id,
                                  const vtss_port_no_t port_no,
                                  vtss_evc_counters_t  *const counters);
vtss_rc vtss_cmn_ece_counters_clear(const vtss_ece_id_t  ece_id,
                                    const vtss_port_no_t port_no);
vtss_rc vtss_cmn_evc_counters_get(const vtss_evc_id_t  evc_id,
                                  const vtss_port_no_t port_no,
                                  vtss_evc_counters_t  *const counters);
vtss_rc vtss_cmn_evc_counters_clear(const vtss_evc_id_t  evc_id,
                                    const vtss_port_no_t port_no);
#endif /* VTSS_SDX_CNT */
#if defined(VTSS_ARCH_CARACAL)
vtss_rc vtss_cmn_mce_add(const vtss_mce_id_t mce_id, const vtss_mce_t *const mce);
vtss_rc vtss_cmn_mce_del(const vtss_mce_id_t mce_id);
#endif /* VTSS_ARCH_CARACAL */
#endif /* VTSS_FEATURE_EVC */

vtss_rc vtss_cmn_debug_info_print(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info);
void vtss_debug_print_header(const vtss_debug_printf_t pr,
                             const char                *header);
void vtss_debug_print_port_header(const vtss_debug_printf_t pr,
                                  const char *txt, u32 count, BOOL nl);
void vtss_debug_print_ports(const vtss_debug_printf_t pr, u8 *member, BOOL nl);
BOOL vtss_debug_group_enabled(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t *const info,
                              const vtss_debug_group_t group);
vtss_rc vtss_debug_print_group(const vtss_debug_group_t group,
                               vtss_rc (* dbg)(const vtss_debug_printf_t pr, 
                                               const vtss_debug_info_t   *const info),
                               const vtss_debug_printf_t pr,
                               const vtss_debug_info_t *const info);
void vtss_debug_print_sticky(const vtss_debug_printf_t pr, 
                             const char *name, u32 value, u32 offset);
void vtss_debug_print_value(const vtss_debug_printf_t pr, const char *name, u32 value);
void vtss_debug_print_reg_header(const vtss_debug_printf_t pr, const char *name);
void vtss_debug_print_reg(const vtss_debug_printf_t pr, const char *name, u32 value);

vtss_rc vtss_cmn_debug_reg_check_set(vtss_state_t *state, BOOL enable);

vtss_rc vtss_cmn_bit_from_one_hot_mask64(u64 mask, u32 *bit_pos);

#if defined (VTSS_FEATURE_PACKET)
vtss_rc vtss_cmn_logical_to_chip_port_mask(const vtss_state_t *const state,
                                                 u64                 logical_port_mask,
                                                 u64                 *chip_port_mask,
                                                 vtss_chip_no_t      *chip_no,
                                                 vtss_port_no_t      *stack_port_no,
                                                 u32                 *port_cnt,
                                                 vtss_port_no_t      *port_no);

vtss_port_no_t vtss_cmn_chip_to_logical_port(const vtss_state_t       *const state,
                                             const vtss_chip_no_t      chip_no,
                                             const vtss_phys_port_no_t chip_port);

vtss_rc vtss_cmn_packet_hints_update(const vtss_state_t          *const state,
                                     const vtss_trace_group_t           trc_grp,
                                     const vtss_etype_t                 etype,
                                           vtss_packet_rx_info_t *const info);
#endif /* VTSS_FEATURE_PACKET */

#endif /* _VTSS_COMMON_H_ */

