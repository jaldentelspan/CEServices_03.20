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

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "port_api.h"
#include "critd_api.h"
#include "ip_dns_api.h"
#include "ip_dns.h"
#include "vtss_dns.h"
#ifdef VTSS_SW_OPTION_IP2
#include "ip2_api.h"
#else
#include "ip_api.h"
#endif /* VTSS_SW_OPTION_IP2 */
#include "packet_api.h"
#include "netdb.h"
#include "misc_api.h"
#include <arpa/inet.h>
#ifdef VTSS_SW_OPTION_ICFG
#include "ip_dns_icfg.h"
#endif /* VTSS_SW_OPTION_ICFG */


#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_IP_DNS

/* Global structure */
static dns_global_conf_t        dns_global_conf;
static BOOL                     vtss_dns_thread_ready = FALSE;
static BOOL                     dns_message_done_flag = FALSE;

#define VTSS_DNS_READY          (_vtss_dns_thread_status_get() == TRUE)
#define VTSS_DNS_BREAK_WAIT(x)  do {    \
    ++(x);                              \
    VTSS_OS_MSLEEP(1973);               \
    if ((x) > 5) break;                 \
} while (0)

#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "IP_DNS",
    .descr     = "IP DNS Module"
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [TRACE_GRP_CRIT] = {
        .name      = "crit",
        .descr     = "Critical regions ",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    }
};
#define DNS_CRIT_ENTER() critd_enter(&dns_global_conf.dns_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define DNS_CRIT_EXIT()  critd_exit( &dns_global_conf.dns_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define DNS_CRIT_ENTER() critd_enter(&dns_global_conf.dns_crit)
#define DNS_CRIT_EXIT()  critd_exit( &dns_global_conf.dns_crit)
#endif /* VTSS_TRACE_ENABLED */

/* Thread variables */
/* DNS Client updating Thread */
static cyg_handle_t vtss_dns_thread_handle;
static cyg_thread   vtss_dns_thread_block;
static char         vtss_dns_thread_stack[THREAD_DEFAULT_STACK_SIZE];
static cyg_handle_t dns_proxy_mbhandle;
static cyg_mbox     dns_proxy_mbox;


/* IP DNS error text */
char *ip_dns_error_txt(ip_dns_error_t rc)
{
    char *txt;

    switch (rc) {
    case IP_DNS_ERROR_GEN:
        txt = "IP DNS generic error";
        break;
    default:
        txt = "IP DNS unknown error";
        break;
    }
    return txt;
}

void dns_pkt_tx(ulong out_port, ushort vid, uchar *frame, size_t len)
{
    uchar *pkt_buf;

    if ((pkt_buf = packet_tx_alloc(len))) {
        packet_tx_props_t tx_props;
        memcpy(pkt_buf, frame, len);
        packet_tx_props_init(&tx_props);

        tx_props.packet_info.modid     = VTSS_MODULE_ID_IP_DNS;
        tx_props.packet_info.frm[0]    = pkt_buf;
        tx_props.packet_info.len[0]    = len;
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(out_port);
#if VTSS_SWITCH_STACKABLE
        if (PORT_NO_IS_STACK(out_port)) {
            tx_props.tx_info.vstax.sym.fwd_mode = VTSS_VSTAX_FWD_MODE_LOOKUP;
            tx_props.tx_info.vstax.sym.ttl      = VTSS_VSTAX_TTL_PORT; // Use Port TTL
            tx_props.tx_info.vstax.sym.prio     = 7;                   // Using the high priority Tx queue
            tx_props.tx_info.vstax.sym.upsid    = 1;
            tx_props.tx_info.vstax.sym.tci.vid  = vid;
            tx_props.tx_info.tx_vstax_hdr       = VTSS_PACKET_TX_VSTAX_SYM;
        }
#endif
        (void)packet_tx(&tx_props);
    }
}

/* Do IP checksum */
static ushort dns_do_ip_chksum(ushort ip_hdr_len, ushort ip_hdr[])
{
    ushort  padd = (ip_hdr_len % 2);
    ushort  word16;
    ulong   sum = 0;
    int     i;

    /* Calculate the sum of all 16 bit words */
    for (i = 0; i < (ip_hdr_len / 2); i++) {
        word16 = ip_hdr[i];
        sum += (ulong)word16;
    }

    /* Add odd byte if needed */
    if (padd == 1) {
        word16 = ip_hdr[(ip_hdr_len / 2)] & 0xFF00;
        sum += (ulong)word16;
    }

    /* Keep only the last 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* One's complement of sum */
    sum = ~sum;

    return ((ushort) sum);
}

/* do UDP checksum */
static ushort dns_do_udp_chksum(ushort udp_len, ushort src_addr[], ushort dest_addr[], ushort udp_hdr[])
{
    ushort  protocol_udp = htons(17);
    ushort  padd = (udp_len % 2);
    ushort  word16;
    ulong   sum = 0;
    int     i;

    /* Calculate the sum of all 16 bit words */
    for (i = 0; i < (udp_len / 2); i++) {
        word16 = udp_hdr[i];
        sum += (ulong)word16;
    }

    /* Add odd byte if needed */
    if (padd == 1) {
        word16 = udp_hdr[(udp_len / 2)] & htons(0xFF00);
        sum += (ulong)word16;
    }

    /* Calculate the UDP pseudo header */
    for (i = 0; i < 2; i++) {   //SIP
        word16 = src_addr[i];
        sum += (ulong)word16;
    }
    for (i = 0; i < 2; i++) {   //DIP
        word16 = dest_addr[i];
        sum += (ulong)word16;
    }
    sum += (ulong)(protocol_udp + htons(udp_len));  //Protocol number and UDP length

    /* Keep only the last 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* One's complement of sum */
    sum = ~sum;

    return ((ushort) sum);
}


static void transmit_dns_response(dns_proxy_query_item_t *dns_proxy_query_msg, ulong qip)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;
    dns_dns_answer          *dns_answer_record;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;
    ip_hdr->ipchksum = 0x0000;

    /* src address */
    ip_hdr->srcipaddr = htonl(dns_proxy_query_msg->querier_dns_dip);

    /* dst address */
    ip_hdr->destipaddr = htonl(dns_proxy_query_msg->querier_ip);


    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);


    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8180);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = htons(1);    /* number of answer entries */
    dns_hdr->nscount = 0;           /* number of authority entries */
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    len = build_qname(ptr, dns_proxy_query_msg->querier_question);
    ptr += len;
    /* Set the type and class fields */
    *ptr++ = (dns_type_a >> 8) & 0xff;
    *ptr++ = dns_type_a & 0xff;
    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* DNS Answer */
    dns_answer_record = (dns_dns_answer *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len];
    dns_answer_record->domain = htons(0xc00c);
    dns_answer_record->rr_type = htons(DNS_TYPE_A);
    dns_answer_record->class = htons(DNS_CLASS_IN);
    dns_answer_record->ttl = htonl(60);
    dns_answer_record->rdlength = htons(4);
    dns_answer_record->rdata[3] = (qip >> 24) & 0xff;
    dns_answer_record->rdata[2] = (qip >> 16) & 0xff;
    dns_answer_record->rdata[1] = (qip >> 8) & 0xff;
    dns_answer_record->rdata[0] = (qip >> 0) & 0xff;
    //sprintf(&dns_answer_record->rdata[0], "%x",(qip >> 24) & 0xff);
    //sprintf(&dns_answer_record->rdata[1], "%x",(qip >> 16) & 0xff);
    //sprintf(&dns_answer_record->rdata[2], "%x",(qip >> 8) & 0xff);
    //sprintf(&dns_answer_record->rdata[3], "%x",(qip >> 0) & 0xff);

    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer));
    /* update hdr checksum */
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = dns_do_ip_chksum(PKT_IP_LEN, (ushort *)&pkt_buf[PKT_LLH_LEN]);

    /* UDP checksum related  */
    udp_hdr->ulen = htons(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer));
    udp_hdr->csum = 0;
    udp_hdr->csum = dns_do_udp_chksum((ushort)(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer)), (ushort *)&pkt_buf[PKT_LLH_LEN + 12], (ushort *)&pkt_buf[PKT_LLH_LEN + 16], (ushort *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN]);

    /* length = eth_len [14] + ip_hdr [20] + igmp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer);

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}


static void transmit_dns_response_rev_lookup(dns_proxy_query_item_t *dns_proxy_query_msg, char *q_host)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len, dns_answer_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;
    uchar                   encoded_name[64];

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;
    ip_hdr->ipchksum = 0x0000;

    /* src address */
    ip_hdr->srcipaddr = htonl(dns_proxy_query_msg->querier_dns_dip);

    /* dst address */
    ip_hdr->destipaddr = htonl(dns_proxy_query_msg->querier_ip);


    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);


    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8080);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = htons(1);    /* number of answer entries */
    dns_hdr->nscount = 0;           /* number of authority entries */
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    len = build_qname(ptr, dns_proxy_query_msg->querier_question);
    ptr += len;
    /* Set the type and class fields */
    *ptr++ = (dns_type_a >> 8) & 0xff;
    *ptr++ = dns_type_a & 0xff;
    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* DNS Answer */
    *ptr++ = 0xc0;
    *ptr++ = 0x0c;
    *ptr++ = 0x00;
    *ptr++ = 0x0c;
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 60;
    len = build_qname(encoded_name, q_host);
    *ptr++ = (len >> 8) & 0xff;
    *ptr++ = len & 0xff;
    memcpy(ptr, encoded_name, len);
    dns_answer_len = len + 12;


    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len);
    /* update hdr checksum */
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = dns_do_ip_chksum(PKT_IP_LEN, (ushort *)&pkt_buf[PKT_LLH_LEN]);

    /* UDP checksum related  */
    udp_hdr->ulen = htons(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = dns_do_udp_chksum((ushort)(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len), (ushort *)&pkt_buf[PKT_LLH_LEN + 12], (ushort *)&pkt_buf[PKT_LLH_LEN + 16], (ushort *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN]);

    /* length = eth_len [14] + ip_hdr [20] + igmp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len;

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}



static void transmit_dns_no_such_name_response(dns_proxy_query_item_t *dns_proxy_query_msg)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len, dns_authority_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;
    ip_hdr->ipchksum = 0x0000;

    /* src address */
    ip_hdr->srcipaddr = htonl(dns_proxy_query_msg->querier_dns_dip);

    /* dst address */
    ip_hdr->destipaddr = htonl(dns_proxy_query_msg->querier_ip);


    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);


    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8183);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = 0;           /* number of answer entries */
    if (dns_proxy_query_msg->querier_opcode == DNS_TYPE_PTR) {
        dns_hdr->nscount = htons(1);    /* number of authority entries */
    } else {
        dns_hdr->nscount = 0;    /* number of authority entries */
    }
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    len = build_qname(ptr, dns_proxy_query_msg->querier_question);
    ptr += len;
    /* Set the type and class fields */
    if (dns_proxy_query_msg->querier_opcode == DNS_TYPE_PTR) {
        dns_type_a = DNS_TYPE_PTR;
        *ptr++ = (dns_type_a >> 8) & 0xff;
        *ptr++ = dns_type_a & 0xff;
    } else {
        *ptr++ = (dns_type_a >> 8) & 0xff;
        *ptr++ = dns_type_a & 0xff;
    }

    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* authority */
    dns_authority_len = 0;
    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len);
    /* update hdr checksum */
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = dns_do_ip_chksum(PKT_IP_LEN, (ushort *)&pkt_buf[PKT_LLH_LEN]);

    /* UDP checksum related  */
    udp_hdr->ulen = htons(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = dns_do_udp_chksum((ushort)(PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len), (ushort *)&pkt_buf[PKT_LLH_LEN + 12], (ushort *)&pkt_buf[PKT_LLH_LEN + 16], (ushort *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN]);
    /* length = eth_len [14] + ip_hdr [20] + igmp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len;

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}


void dns_proxy_send_query_data(dns_proxy_query_item_t *query_item)
{
    dns_proxy_query_item_t *msg;
    if (dns_message_done_flag &&
        dns_global_conf.conf.dns_proxy_status &&
        ((msg = malloc(sizeof(*msg))) != NULL)) {
        *msg = *query_item;
        if (cyg_mbox_put(dns_proxy_mbhandle, msg) != TRUE) {
            free(msg);
        }
    }
}

vtss_rc dns_packet_register(void)
{
    packet_rx_filter_t  filter;

    if (dns_global_conf.filter_id != NULL) {
        return VTSS_OK;
    }

    /* Register for UDP frames via packet API */
    memset(&filter, 0, sizeof(filter));
    filter.modid = VTSS_MODULE_ID_IP_DNS;
    filter.match = PACKET_RX_FILTER_MATCH_IPV4_PROTO;
    filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    filter.ipv4_proto  = 17;
    filter.cb = vtss_rx_dns;
    return packet_rx_filter_register(&filter, &dns_global_conf.filter_id);
}

void dns_packet_unregister(void)
{
    if (dns_global_conf.filter_id != NULL) {
        (void) packet_rx_filter_unregister(dns_global_conf.filter_id);
        dns_global_conf.filter_id = NULL;
    }
}

static BOOL _vtss_dns_thread_status_get(void)
{
    BOOL    status;

    DNS_CRIT_ENTER();
    status = vtss_dns_thread_ready;
    DNS_CRIT_EXIT();

    return status;
}

static void _vtss_dns_thread_status_set(BOOL status)
{
    DNS_CRIT_ENTER();
    vtss_dns_thread_ready = status;
    DNS_CRIT_EXIT();
}

void vtss_dns_thread(cyg_addrword_t data)
{
    struct hostent  *hp;
    u32             skip_cnt, qip;
    struct hostent  *host = NULL;
    struct in_addr  ip;
    char            qname[64];
    char            query[64];
#ifdef VTSS_SW_OPTION_IP2
    vtss_ipv4_t     ipv4_dns;
#else
    ip_status_t     ip_status;
    ip_conf_t       ip_conf;
#endif /* VTSS_SW_OPTION_IP2 */

    skip_cnt = 0;
#ifdef VTSS_SW_OPTION_IP2
    while (!VTSS_DNS_READY) {
#else
    while ((ip_status_get(&ip_status) != VTSS_OK) || !ip_status.ip_thread_ready) {
#endif /* VTSS_SW_OPTION_IP2 */
        VTSS_DNS_BREAK_WAIT(skip_cnt);
    }

    DNS_CRIT_ENTER();
#ifdef VTSS_SW_OPTION_IP2
    if (dns_global_conf.conf.ipv4_dns) {
        cyg_set_current_dns_server((unsigned long)dns_global_conf.conf.ipv4_dns);
    }
#endif /* VTSS_SW_OPTION_IP2 */

    if (dns_global_conf.conf.dns_proxy_status == VTSS_DNS_PROXY_ENABLE) {
        (void) dns_packet_register();
    } else {
        (void) dns_packet_unregister();
    }
    DNS_CRIT_EXIT();

    _vtss_dns_thread_status_set(TRUE);
    T_I("VTSS_DNS_READY");
    while (VTSS_DNS_READY) {
        dns_proxy_query_item_t *dns_proxy_query_msg = cyg_mbox_get(dns_proxy_mbhandle);

        if (msg_switch_is_master() && dns_proxy_query_msg) {
            hp = gethostbyname(dns_proxy_query_msg->querier_question);
            if (hp != NULL) {
                memcpy(&qip, hp->h_addr, hp->h_length);
                transmit_dns_response(dns_proxy_query_msg, qip);
            } else {
                if (dns_proxy_query_msg->querier_opcode == DNS_TYPE_PTR) {
#ifdef VTSS_SW_OPTION_IP2
                    ipv4_dns = 0;
                    if ((ip_dns_mgmt_get_server4(&ipv4_dns) == VTSS_OK) && ipv4_dns) {
                        (void) misc_ipv4_txt(ipv4_dns , query);
#else
                    if (ip_mgmt_conf_cur_get(&ip_conf) == VTSS_OK) {
                        (void) misc_ipv4_txt(ip_conf.ipv4_dns , query);
#endif /* VTSS_SW_OPTION_IP2 */
                    } else {
                        query[0] = '\0';
                    }

                    (void) inet_aton(query, &ip);
                    host = gethostbyaddr( (char *)&ip, sizeof(ip), AF_INET );
                    if ( host ) {
                        strcpy(qname, host->h_name);
                        transmit_dns_response_rev_lookup(dns_proxy_query_msg, qname);
                    } else {
                        transmit_dns_no_such_name_response(dns_proxy_query_msg);
                    }
                } else {
                    transmit_dns_no_such_name_response(dns_proxy_query_msg);
                }
            }
        }

        if (dns_proxy_query_msg) {
            free(dns_proxy_query_msg);
        }
    }
    _vtss_dns_thread_status_set(FALSE);
}

#ifdef VTSS_SW_OPTION_IP2
vtss_rc ip_dns_mgmt_get_server4(vtss_ipv4_t *dns_srv)
{
    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    *dns_srv = dns_global_conf.conf.ipv4_dns;
    DNS_CRIT_EXIT();

    return VTSS_OK;
}

vtss_rc ip_dns_mgmt_set_server4(vtss_ipv4_t *dns_srv)
{
    dns_conf_blk_t  *blk;
    u32             size;

    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    if (dns_global_conf.conf.ipv4_dns == *dns_srv) {
        DNS_CRIT_EXIT();
        return VTSS_OK;
    }
    dns_global_conf.conf.ipv4_dns = *dns_srv;

    if (vtss_dns_thread_ready) {
        cyg_set_current_dns_server((unsigned long)dns_global_conf.conf.ipv4_dns);
    }
    DNS_CRIT_EXIT();

    if ((blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF, &size)) != NULL) {
        blk->conf.ipv4_dns = *dns_srv;
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF);

        return VTSS_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

#ifdef VTSS_SW_OPTION_IPV6
vtss_rc ip_dns_mgmt_get_server6(vtss_ipv6_t *dns_srv)
{
    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    memcpy(dns_srv, &dns_global_conf.conf.ipv6_dns, sizeof(vtss_ipv6_t));
    DNS_CRIT_EXIT();

    return VTSS_OK;
}

vtss_rc ip_dns_mgmt_set_server6(vtss_ipv6_t *dns_srv)
{
    dns_conf_blk_t  *blk;
    u32             size;

    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    if (!memcmp(&dns_global_conf.conf.ipv6_dns, dns_srv, sizeof(vtss_ipv6_t))) {
        DNS_CRIT_EXIT();
        return VTSS_OK;
    }
    memcpy(&dns_global_conf.conf.ipv6_dns, dns_srv, sizeof(vtss_ipv6_t));

#if 0
    /* DNS for IPv6 is not yet supported */
    if (vtss_dns_thread_ready) {
        (void) cyg_set_current_dns_server(&dns_global_conf.conf.ipv6_dns);
    }
#endif
    DNS_CRIT_EXIT();

    if ((blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF, &size)) != NULL) {
        memcpy(&blk->conf.ipv6_dns, dns_srv, sizeof(vtss_ipv6_t));
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF);

        return VTSS_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}
#endif /* VTSS_SW_OPTION_IPV6 */
#endif /* VTSS_SW_OPTION_IP2 */

vtss_rc ip_dns_mgmt_get_proxy_status(BOOL *status)
{
    DNS_CRIT_ENTER();
    *status = dns_global_conf.conf.dns_proxy_status;
    DNS_CRIT_EXIT();

    return VTSS_OK;
}

vtss_rc ip_dns_mgmt_set_proxy_status(BOOL *status)
{
    dns_conf_blk_t  *blk;
    u32             size;

    if (!status) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    if (dns_global_conf.conf.dns_proxy_status == *status) {
        DNS_CRIT_EXIT();
        return VTSS_OK;
    }
    dns_global_conf.conf.dns_proxy_status = *status;

    if (vtss_dns_thread_ready) {
        if (dns_global_conf.conf.dns_proxy_status == VTSS_DNS_PROXY_ENABLE) {
            (void) dns_packet_register();
        } else {
            (void) dns_packet_unregister();
        }
    }
    DNS_CRIT_EXIT();

    if ((blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF, &size)) != NULL) {
        blk->conf.dns_proxy_status = *status;
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF);

        return VTSS_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

static void ip_dns_conf_read(BOOL create)
{
    dns_conf_blk_t      *blk;
    ulong               size;
    BOOL                do_create;
    if ((blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF, &size)) == NULL ||
        size != sizeof(*blk)) {
        blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF, sizeof(*blk));
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        do_create = TRUE;
    } else if (blk->version != DNS_CONF_VERSION) {
        T_W("version mismatch, creating defaults");
        do_create = 1;
    } else {
        do_create = create;
    }

    DNS_CRIT_ENTER();
    if (do_create) {

        /* Use default configuration */
        memset(blk, 0, sizeof(*blk));
        blk->conf.dns_proxy_status = VTSS_DNS_PROXY_DISABLE;
    }
    dns_global_conf.conf = blk->conf;
    if (blk) {
        blk->version = DNS_CONF_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_DNS_CONF);
    }
    DNS_CRIT_EXIT();
}

/* Initialize module */
vtss_rc ip_dns_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    if (data->cmd == INIT_CMD_INIT) {
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
    }
    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

        memset(&dns_global_conf, 0x0, sizeof(dns_global_conf_t));
        vtss_dns_init();

        cyg_mbox_create(&dns_proxy_mbhandle, &dns_proxy_mbox);
        dns_message_done_flag = TRUE;
        /* Create semaphore for critical regions */
        critd_init(&dns_global_conf.dns_crit, "dns_global_conf.dns_crit", VTSS_MODULE_ID_IP_DNS, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        DNS_CRIT_EXIT();
#ifdef VTSS_SW_OPTION_ICFG
        if (ip_dns_icfg_init() != VTSS_OK) {
            T_E("ip_dns_icfg_init failed!");
        }
#endif /* VTSS_SW_OPTION_ICFG */
        break;
    case INIT_CMD_START:
        T_D("START");
        cyg_thread_create(THREAD_BELOW_NORMAL_PRIO,
                          vtss_dns_thread,
                          0,
                          "DNS_Handler",
                          vtss_dns_thread_stack,
                          sizeof(vtss_dns_thread_stack),
                          &vtss_dns_thread_handle,
                          &vtss_dns_thread_block);
        cyg_thread_resume(vtss_dns_thread_handle);
        break;
    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        ip_dns_conf_read(TRUE);
        break;
    case INIT_CMD_MASTER_UP:
        T_D("MASTER_UP");
        ip_dns_conf_read(FALSE);
        _vtss_dns_thread_status_set(TRUE);
        break;
    case INIT_CMD_MASTER_DOWN:
        T_D("MASTER_DOWN");
        if (VTSS_DNS_READY) {
            dns_packet_unregister();
        }
        break;
    case INIT_CMD_SWITCH_ADD:
        T_D("SWITCH_ADD, isid: %d", isid);
        /* Apply all configuration to switch */
        break;
    case INIT_CMD_SWITCH_DEL:
        T_D("SWITCH_DEL, isid: %d", isid);
        break;
    default:
        break;
    }

    T_D("exit");
    return VTSS_OK;
}


/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
