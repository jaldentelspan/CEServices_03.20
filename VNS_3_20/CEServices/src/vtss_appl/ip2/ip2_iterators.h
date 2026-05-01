/*

 Vitesse API software.

 Copyright (c) 2002-2013 Vitesse Semiconductor Corporation "Vitesse". All
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

#ifndef _IP2_ITERATORS_H_
#define _IP2_ITERATORS_H_


#define IP2_ITER_MAX_IPS_OBJS       4
#define IP2_ITER_SINGLE_IPS_OBJS    1

#define IP2_ITER_MAX_IFS_OBJS       IP2_MAX_INTERFACES
#define IP2_ITER_SINGLE_IFS_OBJS    1

/*
    Return first statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_first( const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry);

/*
    Return specific statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_get(   const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry);

/*
    Return next statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_next(  const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry);

/*
    Return first interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_first( const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry);

/*
    Return specific interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_get(   const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry);

/*
    Return next interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_next(  const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry);

/*
    Return first ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_first(  const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry);

/*
    Return specific ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_get(    const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry);

/*
    Return next ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_next(   const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry);

/*
    Return first ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_first(  const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry);

/*
    Return specific ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_get(    const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry);

/*
    Return next ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_next(   const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry);

#endif /* _IP2_ITERATORS_H_ */

