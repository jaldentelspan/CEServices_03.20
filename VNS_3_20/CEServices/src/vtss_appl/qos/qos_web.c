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

#include "web_api.h"
#include "qos_api.h"
#include "msg_api.h"
#include "port_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#ifdef VTSS_FEATURE_QCL_V2
#include "topo_api.h"
#include "mgmt_api.h"
#endif

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
/* Make it look exactly like a Luton26 regarding port policers */
#undef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
#undef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#define QOS_WEB_BUF_LEN 512

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static cyg_int32 handler_stat_qos_counter(CYG_HTTPD_STATE *p)
{
    vtss_isid_t          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t          pit;
    vtss_port_counters_t counters;
    int                  ct;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    /*
    Format: [uport]/[low_queue_receive]/[normal_queue_transmit]/[normal_queue_receive]/[low_queue_transmit]/[medium_queue_receive]/[medium_queue_transmit]/[high_queue_receive]/[high_queue_transmit]|[uport]/[low_queue_receive]/[normal_queue_transmit]/[normal_queue_receive]/[low_queue_transmit]/[medium_queue_receive]/[medium_queue_transmit]/[high_queue_receive]/[high_queue_transmit]|...
    */
    cyg_httpd_start_chunked("html");

    if (VTSS_ISID_LEGAL(sid) && msg_switch_exists(sid)) {
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
        while (port_iter_getnext(&pit)) {
            if (cyg_httpd_form_varable_find(p, "clear")) { /* Clear? */
                if (port_mgmt_counters_clear(sid, pit.iport) != VTSS_OK) {
                    T_W("Unable to clear counters for sid %u, port %u", sid, pit.uport);
                    break;
                }
                memset(&counters, 0, sizeof(counters)); /* Cheating a little... */
            } else {
                /* Normal read */
                if (port_mgmt_counters_get(sid, pit.iport, &counters) != VTSS_OK) {
                    T_W("Unable to get counters for sid %u, port %u", sid, pit.uport);
                    break;              /* Most likely stack error - bail out */
                }
            }
            /* Output the counters */
#ifdef VTSS_ARCH_LUTON28
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu%s",
                          pit.uport,
                          counters.prop.rx_prio[0],
                          counters.prop.tx_prio[0],
                          counters.prop.rx_prio[1],
                          counters.prop.tx_prio[1],
                          counters.prop.rx_prio[2],
                          counters.prop.tx_prio[2],
                          counters.prop.rx_prio[3],
                          counters.prop.tx_prio[3],
                          pit.last ? "" : "|");
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu%s",
                          pit.uport,
                          counters.prop.rx_prio[0],
                          counters.prop.tx_prio[0],
                          counters.prop.rx_prio[1],
                          counters.prop.tx_prio[1],
                          counters.prop.rx_prio[2],
                          counters.prop.tx_prio[2],
                          counters.prop.rx_prio[3],
                          counters.prop.tx_prio[3],
                          counters.prop.rx_prio[4],
                          counters.prop.tx_prio[4],
                          counters.prop.rx_prio[5],
                          counters.prop.tx_prio[5],
                          counters.prop.rx_prio[6],
                          counters.prop.tx_prio[6],
                          counters.prop.rx_prio[7],
                          counters.prop.tx_prio[7],
                          pit.last ? "" : "|");
#endif
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
static cyg_int32 handler_config_stormctrl(CYG_HTTPD_STATE *p)
{
    int          ct, var_rate;
    qos_conf_t   conf, newconf;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (qos_conf_get(&conf) < 0) {
            errors++;   /* Probably stack error */
        } else {
            newconf = conf;

            //unicast
            if (cyg_httpd_form_varable_find(p, "status_0")) { /* "on" if checked */
                newconf.policer_uc_status = RATE_STATUS_ENABLE;
            } else {
                newconf.policer_uc_status = RATE_STATUS_DISABLE;
            }
            if (cyg_httpd_form_varable_int(p, "rate_0", &var_rate)) {
                newconf.policer_uc = var_rate;
            }

            //multicast
            if (cyg_httpd_form_varable_find(p, "status_1")) { /* "on" if checked */
                newconf.policer_mc_status = RATE_STATUS_ENABLE;
            } else {
                newconf.policer_mc_status = RATE_STATUS_DISABLE;
            }
            if (cyg_httpd_form_varable_int(p, "rate_1", &var_rate)) {
                newconf.policer_mc = var_rate;
            }

            //broadcast
            if (cyg_httpd_form_varable_find(p, "status_2")) { /* "on" if checked */
                newconf.policer_bc_status = RATE_STATUS_ENABLE;
            } else {
                newconf.policer_bc_status = RATE_STATUS_DISABLE;
            }
            if (cyg_httpd_form_varable_int(p, "rate_2", &var_rate)) {
                newconf.policer_bc = var_rate;
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                vtss_rc rc;
                if ((rc = qos_conf_set(&newconf)) < 0) {
                    T_D("qos_conf_set: failed rc = %d", rc);
                    errors++; /* Probably stack error */
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/stormctrl.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        /* should get these values from management APIs */
        if (qos_conf_get(&conf) == VTSS_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u|%d/%u|%d/%u",
                          conf.policer_uc_status,
                          conf.policer_uc,
                          conf.policer_mc_status,
                          conf.policer_mc,
                          conf.policer_bc_status,
                          conf.policer_bc);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

/****************************************************************************/
/*  JAGUAR/Luton26/Serval code starts here                                  */
/****************************************************************************/
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
#include "qos_features.h"
#ifdef  _VTSS_QOS_FEATURES_H_ // This is just a hack in order to keep lint happy :-)
#endif

#ifdef VTSS_FEATURE_QCL_V2
//***qce handler
static char *qcl_qce_port_list_txt(qos_qce_entry_conf_t *conf, char *buf)
{
    BOOL          port_list[VTSS_PORTS];
    int           i;
    char          *ret_buf;

    for (i = VTSS_PORT_NO_START; i < VTSS_PORT_NO_END; i++) {
        if (VTSS_PORT_BF_GET(conf->port_list, i)) {
            port_list[i] = 1;
        } else {
            port_list[i] = 0;
        }
    }

    ret_buf = mgmt_iport_list2txt(port_list, buf);
    if (strlen(ret_buf) == 0) {
        strcpy(buf, "None");
    }
    return buf;
}

static char *qcl_qce_ipv6_txt(vtss_qce_u32_t *ip, char *buf)
{
    if (!ip->mask[0] && !ip->mask[1] && !ip->mask[2] && !ip->mask[3]) {
        sprintf(buf, "Any");
    } else {
        sprintf(buf, "%u.%u.%u.%u", ip->value[0], ip->value[1],
                ip->value[2], ip->value[3]);
    }
    return (buf);
}

static char *qcl_qce_range_txt_u16(qos_qce_vr_u16_t *range, char *buf)
{
    if (range->in_range) {
        sprintf(buf, "%u-%u", range->vr.r.low, range->vr.r.high);
    } else if (range->vr.v.mask) {
        sprintf(buf, "%u", range->vr.v.value);
    } else {
        sprintf(buf, "Any");
    }
    return (buf);
}

static cyg_int32 handler_config_qcl_v2(CYG_HTTPD_STATE *p)
{
    vtss_qce_id_t        qce_id = QCE_ID_NONE;
    vtss_qce_id_t        next_qce_id = QCE_ID_NONE;
    vtss_qcl_id_t        qcl_id = QCL_ID_START;
    int                  qce_flag = 0;
    int                  var_value1 = 0, var_value2 = 0, var_value3 = 0, var_value4 = 0;
    qos_qce_entry_conf_t qce_conf, newconf, qce_next;
    const char           *var_string;
    BOOL                 first;
    int                  i;
    BOOL                 add;
    char                 str_buff1[24], str_buff2[24];
    vtss_port_no_t       iport;
    const i8             *str;
    size_t               len;
    int                  ct;
    switch_iter_t        sit;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        char search_str[32];
        memset(&qce_conf, 0x0, sizeof(qce_conf));
        memset(&newconf, 0x0, sizeof(newconf));
        memset(&qce_next, 0x0, sizeof(qce_next));
        newconf.isid = VTSS_ISID_GLOBAL;

        //switch_id
#if VTSS_SWITCH_STACKABLE
        if (cyg_httpd_form_varable_int(p, "portMemSIDSelect", &var_value1)) {//Specific sid
            if (var_value1 != -1) {
                newconf.isid = topo_usid2isid(var_value1);
                if (redirectUnmanagedOrInvalid(p, newconf.isid)) {/* Redirect unmanaged/invalid access to handler */
                    return -1;
                }
            }
        }
#endif /* VTSS_SWITCH_STACKABLE */

        //qce_id, next_qce_id
        qce_id = QCE_ID_NONE;
        if (cyg_httpd_form_varable_int(p, "qce_id", &var_value1)) {
            qce_id = var_value1;
        }
        add = (qce_id == QCE_ID_NONE ||
               qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_conf, 0) != VTSS_OK);
        if (cyg_httpd_form_varable_int(p, "next_qce_id", &var_value1)) {
            next_qce_id = var_value1;
        }
        if (add == FALSE && next_qce_id == QCE_ID_NONE) {//get next entry when qce alreay exists & next_qce_id is none
            if (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_next, 1) == VTSS_OK) {
                next_qce_id = qce_next.id;
            }
        }
        if (next_qce_id == QCE_ID_END) {//if entry being edited is at last then set qce_next_id to none i.e to 0
            next_qce_id = QCE_ID_NONE;
        }
        newconf.id = qce_id;
        //Key Key -> Frame Type
        if (cyg_httpd_form_varable_int(p, "frame_type", &var_value1)) {
            switch (var_value1) {
            case 0:
                newconf.type = VTSS_QCE_TYPE_ANY;
                break;
            case 1:
                newconf.type = VTSS_QCE_TYPE_ETYPE;
                if (cyg_httpd_form_varable_int(p, "ether_type_filter", &var_value2)) {
                    newconf.key.frame.etype.etype.mask[0] = newconf.key.frame.etype.etype.mask[1] = 0;
                    if (var_value2 == 1) {//i.e etype is Specific value
                        if (cyg_httpd_form_varable_hex(p, "ether_type", (ulong *)&var_value3)) {
                            newconf.key.frame.etype.etype.value[0] = (var_value3 >> 8) & 0xFF;
                            newconf.key.frame.etype.etype.value[1] = var_value3 & 0xFF;
                            newconf.key.frame.etype.etype.mask[0] = newconf.key.frame.etype.etype.mask[1] = 0xFF;
                        }
                    }
                }
                break;
            case 2:
                newconf.type = VTSS_QCE_TYPE_LLC;
                break;
            case 3:
                newconf.type = VTSS_QCE_TYPE_SNAP;
                break;
            case 4:
                newconf.type = VTSS_QCE_TYPE_IPV4;
                break;
            case 5:
                newconf.type = VTSS_QCE_TYPE_IPV6;
                break;
            default:
                T_E("Invalid frame type value");
                break;
            }
        }
        //Look for por list in post query
        for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
            sprintf(search_str, "new_ckBox_%d", iport2uport(iport));
            VTSS_PORT_BF_SET(newconf.port_list, iport, 0);
            if (cyg_httpd_form_varable_find(p, search_str)) { // "on" if checked
                VTSS_PORT_BF_SET(newconf.port_list, iport, 1);
            }
        }
        //qce key tag
        QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_VLAN_TAG, VTSS_VCAP_BIT_ANY);
        if (cyg_httpd_form_varable_int(p, "qceKeyTag", &var_value1)) {
            if (var_value1 == 2) {//i.e. vlan.tagged == "tag"
                QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_VLAN_TAG, VTSS_VCAP_BIT_1);
            }
            if (var_value1 == 1) {//i.e. vlan.tagged == "untagged"
                QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_VLAN_TAG, VTSS_VCAP_BIT_0);
            }
        }
        //vid specific|Range fields
        if (cyg_httpd_form_varable_int(p, "KeyVIDSelect", &var_value1)) {
            if (var_value1 == 0) {//Any
                newconf.key.vid.in_range = FALSE;
                newconf.key.vid.vr.v.value = 0;
                newconf.key.vid.vr.v.mask = 0;
            }
            if (var_value1 != 0) {
                if (cyg_httpd_form_varable_int(p, "KeyVIDSpecific", &var_value1)) {//Specific
                    newconf.key.vid.in_range = FALSE;
                    newconf.key.vid.vr.v.value = var_value1;
                    newconf.key.vid.vr.v.mask = 0xFFFF;
                }
                if (cyg_httpd_form_varable_int(p, "KeyVidStart", &var_value1)) {
                    newconf.key.vid.in_range = TRUE;
                    newconf.key.vid.vr.r.low = var_value1;
                    if (cyg_httpd_form_varable_int(p, "KeyVidLast", &var_value2)) {
                        newconf.key.vid.vr.r.high = var_value2;
                    }
                }
            }
        }
        //Key -> pcp field
        if (cyg_httpd_form_varable_int(p, "qceKeyPCPSelect", &var_value1)) {
            if (var_value1 == 0) {
                newconf.key.pcp.mask = 0;
                newconf.key.pcp.value = var_value1;
            } else if (var_value1 > 0 && var_value1 <= 8) {//pcp values other than 'any'
                newconf.key.pcp.value = (var_value1 - 1);
                newconf.key.pcp.mask = 0xFF;
            } else if (var_value1 > 8 && var_value1 <= 12) {
                newconf.key.pcp.value = (var_value1 - 9) * 2;
                newconf.key.pcp.mask = 0xFE;
            } else {
                newconf.key.pcp.value = (var_value1 == 13) ? 0 : 4;
                newconf.key.pcp.mask = 0xFC;
            }
        }
        // Key -> dei
        if (cyg_httpd_form_varable_int(p, "qceKeyDEISelect", &var_value1)) {
            QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_VLAN_DEI, var_value1);
        }
        // Key -> smac address
        if (cyg_httpd_form_varable_int(p, "qceKeySMACSelect", &var_value1)) {
            for (i = 0; i < 3; i++) {
                newconf.key.smac.mask[i] = 0x00;
            }
            uint mac[3];
            if (var_value1 == 1) {//smac value will be present when var_value1 is == 1(Specific)
                sprintf(search_str, "qceKeySMACValue");
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (sscanf(str, "%2x-%2x-%2x", &mac[0], &mac[1], &mac[2]) == 3) {
                        for (i = 0; i < 3; i++) {
                            newconf.key.smac.value[i] = mac[i];
                            newconf.key.smac.mask[i] = 0xFF;
                        }
                    }
                }
            }
        }
        // Key -> DMAC type
        if (cyg_httpd_form_varable_int(p, "qceKeyDMACTypeSelect", &var_value1)) {
            QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_DMAC_TYPE, var_value1);
        }
        //Key Key -> Frame Type
        if (cyg_httpd_form_varable_int(p, "frame_type", &var_value1)) {
            switch (var_value1) {
            case VTSS_QCE_TYPE_ANY://frame_type == Any
                break;
            case VTSS_QCE_TYPE_ETYPE://frame_type == Ethernet
                //etype.value already assinged earlier
                break;
            case VTSS_QCE_TYPE_LLC://frame_type == LLC
                // SSAP Address
                if (cyg_httpd_form_varable_int(p, "SSAPAddrSel", &var_value1)) {
                    newconf.key.frame.llc.ssap.value = 0;
                    newconf.key.frame.llc.ssap.mask = 0;
                    if (var_value1 == 1) {
                        if (cyg_httpd_form_varable_hex(p, "LLCSSAPVal", (ulong *)&var_value1)) {
                            newconf.key.frame.llc.ssap.value = var_value1;
                            newconf.key.frame.llc.ssap.mask = 0xFF;
                        }
                    }
                }
                // DSAP Address
                if (cyg_httpd_form_varable_int(p, "DSAPAddrSel", &var_value1)) {
                    newconf.key.frame.llc.dsap.value = 0;
                    newconf.key.frame.llc.dsap.mask = 0;
                    if (var_value1 == 1) {
                        if (cyg_httpd_form_varable_hex(p, "LLCDSAPVal", (ulong *)&var_value1)) {
                            newconf.key.frame.llc.dsap.value = var_value1;
                            newconf.key.frame.llc.dsap.mask = 0xFF;
                        }
                    }
                }
                // LLC Control
                if (cyg_httpd_form_varable_int(p, "LLCCntrlSel", &var_value1)) {
                    newconf.key.frame.llc.control.value = 0;
                    newconf.key.frame.llc.control.mask = 0;
                    if (var_value1 == 1) {
                        if (cyg_httpd_form_varable_hex(p, "LLCControlVal", (ulong *)&var_value1)) {
                            newconf.key.frame.llc.control.value = var_value1;
                            newconf.key.frame.llc.control.mask = 0xFF;
                        }
                    }
                }
                break;
            case VTSS_QCE_TYPE_SNAP://frame_type == SNAP
                if (cyg_httpd_form_varable_int(p, "SNAP_Ether_type_filter", &var_value2)) {
                    if (var_value2 == 1) {// if ether_type is specific
                        if (cyg_httpd_form_varable_hex(p, "ether_type", (ulong *)&var_value3)) {
                            newconf.key.frame.snap.pid.mask[0] = newconf.key.frame.snap.pid.mask[1] = 0xff;
                            newconf.key.frame.snap.pid.value[0] = var_value3 >> 8;
                            newconf.key.frame.snap.pid.value[1] = var_value3 & 0xff;
                        } else {// if ether_type is Any
                            newconf.key.frame.snap.pid.mask[0] = newconf.key.frame.snap.pid.mask[1] = 0;
                        }
                    }
                }
                break;
            case VTSS_QCE_TYPE_IPV4://frame_type == IPv4
                if (cyg_httpd_form_varable_int(p, "protocol_filterIPv4", &var_value1)) {
                    switch (var_value1) {
                    case 0://Protocol Type is Any
                        newconf.key.frame.ipv4.proto.mask = 0;
                        break;
                    case 1://Protocol Type is UDP
                        newconf.key.frame.ipv4.sport.in_range = newconf.key.frame.ipv4.dport.in_range = FALSE;
                        newconf.key.frame.ipv4.sport.vr.v.mask = newconf.key.frame.ipv4.dport.vr.v.mask = 0;
                        newconf.key.frame.ipv4.sport.vr.v.value = newconf.key.frame.ipv4.dport.vr.v.value = 0;
                        newconf.key.frame.ipv4.proto.value = 17;//UDP protocol number
                        newconf.key.frame.ipv4.proto.mask = 0xFF;// for specific value
                        if (cyg_httpd_form_varable_int(p, "keySportSelect", &var_value2)) {
                            if (var_value2 != 0) {//if Specific
                                if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv4.sport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv4.sport.vr.v.value = var_value3;
                                }
                                //if in range
                                if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3)) {
                                    newconf.key.frame.ipv4.sport.in_range = TRUE;
                                    newconf.key.frame.ipv4.sport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                        newconf.key.frame.ipv4.sport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        if (cyg_httpd_form_varable_int(p, "keyDportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {//if Specific
                                    newconf.key.frame.ipv4.dport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv4.dport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3)) {
                                    newconf.key.frame.ipv4.dport.in_range = TRUE;
                                    newconf.key.frame.ipv4.dport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                        newconf.key.frame.ipv4.dport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        break;
                    case 2://Protocol Type is TCP
                        newconf.key.frame.ipv4.sport.in_range = newconf.key.frame.ipv4.dport.in_range = FALSE;
                        newconf.key.frame.ipv4.sport.vr.v.mask = newconf.key.frame.ipv4.dport.vr.v.mask = 0;
                        newconf.key.frame.ipv4.sport.vr.v.value = newconf.key.frame.ipv4.dport.vr.v.value = 0;

                        newconf.key.frame.ipv4.proto.value = 6;//TCP protocol number
                        newconf.key.frame.ipv4.proto.mask = 0xFF;// for specific value
                        if (cyg_httpd_form_varable_int(p, "keySportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv4.sport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv4.sport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3)) {
                                    newconf.key.frame.ipv4.sport.in_range = TRUE;
                                    newconf.key.frame.ipv4.sport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                        newconf.key.frame.ipv4.sport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        if (cyg_httpd_form_varable_int(p, "keyDportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv4.dport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv4.dport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3)) {
                                    newconf.key.frame.ipv4.dport.in_range = TRUE;
                                    newconf.key.frame.ipv4.dport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                        newconf.key.frame.ipv4.dport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        break;
                    case 3://Protocol Type is Other
                        if (cyg_httpd_form_varable_int(p, "KeyProtoNbr", &var_value1)) {
                            newconf.key.frame.ipv4.proto.value = var_value1;//Other protocol number
                            newconf.key.frame.ipv4.proto.mask = 0xFF;// for specific value
                        }
                        break;
                    default:
                        // through an error saying invalid input
                        break;
                    }

                }//IPv4 protocol filter block
                // ipv4 set sip
                newconf.key.frame.ipv4.sip.mask = 0;//IPv4IPAddrSelect
                if (cyg_httpd_form_varable_int(p, "IPv4IPAddrSelect", &var_value1) == 1) {
                    if (cyg_httpd_form_varable_ipv4(p, "IPv4Addr", &newconf.key.frame.ipv4.sip.value)) {
                        (void)cyg_httpd_form_varable_ipv4(p, "IPMaskValue", &newconf.key.frame.ipv4.sip.mask);
                    }
                }
                //set ipv4 fragment (Any:0, Yes:1, No:2) - name:value
                if (cyg_httpd_form_varable_int(p, "IPv4IPfragMenu", &var_value1)) {
                    if (var_value1 == 0) {//fragment == "Any"
                        QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_IPV4_FRAGMENT, VTSS_VCAP_BIT_ANY);
                    }
                    if (var_value1 == 1) {// fragment == "Yes"
                        QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_IPV4_FRAGMENT, VTSS_VCAP_BIT_1);
                    }
                    if (var_value1 == 2) {// fragment == "No"
                        QCE_ENTRY_CONF_KEY_SET(newconf.key.key_bits, QOS_QCE_IPV4_FRAGMENT, VTSS_VCAP_BIT_0);
                    }
                }
                //dscp choice field
                newconf.key.frame.ipv4.dscp.in_range = FALSE;
                newconf.key.frame.ipv4.dscp.vr.v.value = 0;
                newconf.key.frame.ipv4.dscp.vr.v.mask = 0;
                if (cyg_httpd_form_varable_int(p, "IPv4DSCPChoice", &var_value1)) {
                    if (var_value1 == 1) {//Specific value
                        if (cyg_httpd_form_varable_int(p, "keyDSCPSpcValue", &var_value2)) {
                            newconf.key.frame.ipv4.dscp.vr.v.value = var_value2;
                            newconf.key.frame.ipv4.dscp.vr.v.mask = 0x3F;
                        }
                    }
                    if (var_value1 == 2) {//Range of values
                        if (cyg_httpd_form_varable_int(p, "keyDSCPRngStart", &var_value2)) {
                            newconf.key.frame.ipv4.dscp.in_range = TRUE;
                            newconf.key.frame.ipv4.dscp.vr.r.low = var_value2;
                            if (cyg_httpd_form_varable_int(p, "keyDSCPRngLast", &var_value3)) {
                                newconf.key.frame.ipv4.dscp.vr.r.high = var_value3;
                            }
                        }
                    }
                }
                break;
            case VTSS_QCE_TYPE_IPV6://frame_type == IPv6
                if (cyg_httpd_form_varable_int(p, "protocol_filterIPv6", &var_value1)) {
                    switch (var_value1) {
                    case 0://Protocol Type is Any
                        newconf.key.frame.ipv6.proto.mask = 0;
                        newconf.key.frame.ipv6.proto.value = 0;
                        break;
                    case 1://Protocol Type is UDP
                        newconf.key.frame.ipv6.sport.in_range =  newconf.key.frame.ipv6.dport.in_range = FALSE;
                        newconf.key.frame.ipv6.sport.vr.v.mask = newconf.key.frame.ipv6.dport.vr.v.mask = 0;
                        newconf.key.frame.ipv6.sport.vr.v.value = newconf.key.frame.ipv6.dport.vr.v.value = 0;
                        newconf.key.frame.ipv6.proto.value = 17;//UDP protocol number
                        newconf.key.frame.ipv6.proto.mask = 0xFF;
                        if (cyg_httpd_form_varable_int(p, "keySportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv6.sport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv6.sport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3)) {
                                    newconf.key.frame.ipv6.sport.in_range = TRUE;
                                    newconf.key.frame.ipv6.sport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                        newconf.key.frame.ipv6.sport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        if (cyg_httpd_form_varable_int(p, "keyDportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv6.dport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv6.dport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3)) {
                                    newconf.key.frame.ipv6.dport.in_range = TRUE;
                                    newconf.key.frame.ipv6.dport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                        newconf.key.frame.ipv6.dport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        break;
                    case 2://Protocol Type is TCP
                        newconf.key.frame.ipv6.sport.in_range =  newconf.key.frame.ipv6.dport.in_range = FALSE;
                        newconf.key.frame.ipv6.sport.vr.v.mask = newconf.key.frame.ipv6.dport.vr.v.mask = 0;
                        newconf.key.frame.ipv6.sport.vr.v.value = newconf.key.frame.ipv6.dport.vr.v.value = 0;
                        newconf.key.frame.ipv6.proto.value = 6;//UDP protocol number
                        newconf.key.frame.ipv6.proto.mask = 0xFF;
                        if (cyg_httpd_form_varable_int(p, "keySportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv6.sport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv6.sport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3)) {
                                    newconf.key.frame.ipv6.sport.in_range = TRUE;
                                    newconf.key.frame.ipv6.sport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                        newconf.key.frame.ipv6.sport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        if (cyg_httpd_form_varable_int(p, "keyDportSelect", &var_value2)) {
                            if (var_value2 != 0) {
                                if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {
                                    newconf.key.frame.ipv6.dport.vr.v.mask = 0xFFFF;
                                    newconf.key.frame.ipv6.dport.vr.v.value = var_value3;
                                }
                                if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3)) {
                                    newconf.key.frame.ipv6.dport.in_range = TRUE;
                                    newconf.key.frame.ipv6.dport.vr.r.low = var_value3;
                                    if (cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                        newconf.key.frame.ipv6.dport.vr.r.high = var_value4;
                                    }
                                }
                            }
                        }
                        break;
                    case 3://Protocol Type is Other
                        if (cyg_httpd_form_varable_int(p, "KeyProtoNbr", &var_value1)) {
                            newconf.key.frame.ipv6.proto.value = var_value1;
                            newconf.key.frame.ipv6.proto.mask = 0xFF;
                        }
                        break;
                    default:
                        // through an error saying invalid input
                        break;
                    }
                }//IPv6 Protocol filter block
                // ipv6 set sip
                uint ipAddr[4], ipMask[4];//IPv6IPAddrSelect
                if (cyg_httpd_form_varable_int(p, "IPv6IPAddrSelect", &var_value1)) {
                    if (var_value1 == 1) {//Specific ipaddress value
                        sprintf(search_str, "IPv6Addr");
                        if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                            if (sscanf(str, "%3u.%3u.%3u.%3u", &ipAddr[0], &ipAddr[1], &ipAddr[2], &ipAddr[3]) == 4) {
                                if ((str = cyg_httpd_form_varable_string(p, "IPMaskValue", &len)) != NULL) {
                                    if (sscanf(str, "%3u.%3u.%3u.%3u", &ipMask[0], &ipMask[1], &ipMask[2], &ipMask[3]) == 4) {
                                        for (i = 0; i < 4; i++) {
                                            newconf.key.frame.ipv6.sip.value[i] = ipAddr[i] & 0xFF;
                                            newconf.key.frame.ipv6.sip.mask[i] = ipMask[i];
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        for (i = 0; i < 4; i++) {
                            newconf.key.frame.ipv6.sip.mask[i] = 0;
                        }
                    }
                }
                //dscp choice field
                newconf.key.frame.ipv6.dscp.in_range = FALSE;
                newconf.key.frame.ipv6.dscp.vr.v.mask = 0;
                newconf.key.frame.ipv6.dscp.vr.v.value = 0;
                if (cyg_httpd_form_varable_int(p, "IPv6DSCPChoice", &var_value1)) {
                    if (var_value1 == 1) {
                        if (cyg_httpd_form_varable_int(p, "keyDSCPSpcValue", &var_value2)) {
                            newconf.key.frame.ipv6.dscp.vr.v.mask = 0x3F;
                            newconf.key.frame.ipv6.dscp.vr.v.value = var_value2;
                        }
                    }
                    if (var_value1 == 2) {
                        if (cyg_httpd_form_varable_int(p, "keyDSCPRngStart", &var_value2)) {
                            newconf.key.frame.ipv6.dscp.in_range = TRUE;
                            newconf.key.frame.ipv6.dscp.vr.r.low = var_value2;
                            if (cyg_httpd_form_varable_int(p, "keyDSCPRngLast", &var_value3)) {
                                newconf.key.frame.ipv6.dscp.vr.r.high = var_value3;
                            }
                        }
                    }
                }
                break;
            default:
                //through an error unexpected value in var_value1
                break;
            }// frame type switch case block
        }// frame type

        //qce action configuration
        //action configuration Format-(default:0, class 0:1 .. class 7:8) - Class:var_value1
        if (cyg_httpd_form_varable_int(p, "actionClasSel",  &var_value1)) {
            if (var_value1 != 0) {
                QCE_ENTRY_CONF_ACTION_SET(newconf.action.action_bits, QOS_QCE_ACTION_PRIO, 1);
                newconf.action.prio = (var_value1 - 1);
            }
        }
        // DP
        if (cyg_httpd_form_varable_int(p, "actionDPSel",  &var_value1)) {
            if (var_value1 != 0) {
                QCE_ENTRY_CONF_ACTION_SET(newconf.action.action_bits, QOS_QCE_ACTION_DP, 1);
                newconf.action.dp = (var_value1 - 1);
            }
        }
        // DSCP
        if (cyg_httpd_form_varable_int(p, "actionDSCPSel", &var_value1)) {
            QCE_ENTRY_CONF_ACTION_SET(newconf.action.action_bits, QOS_QCE_ACTION_DSCP, 0);
            if (var_value1 != 0) {
                QCE_ENTRY_CONF_ACTION_SET(newconf.action.action_bits, QOS_QCE_ACTION_DSCP, 1);
                newconf.action.dscp = var_value1 - 1;
            }
        }
        // add the qce entry finally
        if (qos_mgmt_qce_entry_add(QCL_USER_STATIC, QCL_ID_END, next_qce_id, &newconf) == VTSS_OK) {
            T_D("QCE ID %d %s ", newconf.id, add ? "added" : "modified");
            if (next_qce_id) {
                T_D("before QCE ID %d\n", next_qce_id);
            } else {
                T_D("last\n");
            }
            redirect(p, errors ? STACK_ERR_URL : "/qcl_v2.htm");
        } else {
            T_E("QCL Add failed\n");
            redirect(p, errors ? STACK_ERR_URL : "/qcl_v2.htm?error=1");
        }
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        /*
           Format:
           <Del>         : qceConfigFlag=1 and qce_id=<qce_id to delete>
           <Move>        : qceConfigFlag=2 and qce_id=<qce_id to move>
           <Edit>        : qceConfigFlag=3 and qce_id=<qce_id to edit>
           <Insert>      : qceConfigFlag=4 and qce_id=<next_qce_id to insert before>
           <Add to Last> : qceConfigFlag=4 and qce_id=0 (inserts last if next_qce_id=0)
        */
        u8   bit_val;
        char buf[MGMT_PORT_BUF_SIZE];
        cyg_httpd_start_chunked("html");
        vtss_isid_t isid = VTSS_ISID_GLOBAL;
        if ((var_string = cyg_httpd_form_varable_find(p, "qceConfigFlag")) != NULL) {
            qce_flag = atoi(var_string);
            if ((var_string = cyg_httpd_form_varable_find(p, "qce_id")) != NULL) {
                qce_id = atoi(var_string);
            }
            switch (qce_flag) {
            case 1://<Del> qce entry
                (void) qos_mgmt_qce_entry_del(isid, QCL_USER_STATIC, QCL_ID_END, qce_id);
                break;
            case 2://<Move> qce entry
                if (cyg_httpd_form_varable_int(p, "qce_id",  &var_value1)) {
                    qce_id = var_value1;
                    if (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_conf, 0) == VTSS_OK) {
                        if (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC, QCL_ID_END, qce_id, &newconf, 1) == VTSS_OK) {
                            if (qos_mgmt_qce_entry_add(QCL_USER_STATIC, QCL_ID_END, qce_id, &newconf) != VTSS_OK) {
                                T_W("qos_mgmt_qce_entry_add() failed");
                            }
                        }
                    } else {
                        T_W ("qos_mgmt_qce_entry_get() failed");
                    }
                }
                break;
            case 3://<Edit> This format is for 'qos/html/qcl_v2_edit.htm' webpage
            case 4://<Insert> or <Add to Last> This format is for 'qos/html/qcl_v2_edit.htm' webpage
                // Create a list of present switches in usid order separated by "#"
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
                while (switch_iter_getnext(&sit)) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d", sit.first ? "" : "#", sit.usid);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                if (qce_flag == 4) {
                    cyg_httpd_write_chunked(",-1", 3);
                    cyg_httpd_end_chunked();
                    return -1; // return from <Insert> or <Add to Last> case
                }
                /* Format:
                 * <qce_info>;<frame_info>
                 *
                 * qce_info        :== <sid_next>/<port_list>/<tag>/<vid>/<pcp>/<dei>/<smac>/<dmac_type>/<act_class>/<act_dpl>/<act_dscp>
                 *   sid_next      :== <usid_list>,<usid>,<next_qce_id>
                 *     usid_list   :== <usid l>#<usid m>#<usid n> // List of present (active) switches in usid order.
                 *     usid        :== 1..16 or -1 for all switches
                 *     next_qce_id :== 0..256
                 *   port_list     :== list of disabled uports separated by ','. Empty list ~ All ports enabled
                 *   tag           :== 0: Any, 1: Untag, 2: Tag
                 *   vid           :== <vid_low>,<vid_high>
                 *     vid_low     :== -1 or 1..4095  // -1: Any
                 *     vid_high    :== -1 or 1..4095  // -1: Use vid_low as specific, else high range.
                 *   pcp           :== <pcp_low>,<pcp_high>
                 *     pcp_low     :== -1..7   // -1 is Any
                 *     pcp_high    :== -1..7    // Not used if pcp_low == -1. If pcp_low != pcp_high then it is a range, else use pcp_low.
                 *   dei           :== -1: Any , else dei value
                 *   smac          :== String  // "Any" or "xx-xx-xx-xx-xx-xx"
                 *   dmac_type     :== 0..3    // One of qos_qce_dmac_type_t
                 *   act_class     :== -1..7   // -1 is no action, else classify to selected value
                 *   act_dpl       :== -1..3   // -1 is no action, else classify to selected value
                 *   act_dscp      :== -1..63  // -1 is no action, else classify to selected value
                 *
                 * frame_info      :== <frame_type>/<type_any> or <type_eth> or <type_llc> or <type_snap> or <type_ipv4> or <type_ipv6>
                 *   frame_type    :== 0..5    // One of vtss_qce_type_t
                 *   type_any      :== String "Any"
                 *   type_eth      :== <eth_spec>/<eth_val>
                 *     eth_spec    :== 0 or 1 where 0 is Any and 1 is use eth_val
                 *     eth_val     :== 0 or 0600..ffff - value in hex (without 0x prepended)
                 *   type_llc      :== <ssap_spec>/<dsap_spec>/<ctrl_spec>/<ssap_val>/<dsap_val>/<ctrl_val>
                 *     ssap_spec   :== -1 for Any, else value in decimal
                 *     dsap_spec   :== -1 for Any, else value in decimal
                 *     ctrl_spec   :== -1 for Any, else value in decimal
                 *     ssap_val    :== 0 or value in hex (without 0x prepended)
                 *     dsap_val    :== 0 or value in hex (without 0x prepended)
                 *     ctrl_val    :== 0 or value in hex (without 0x prepended)
                 *   type_snap     :== <snap_spec>/<snap_hbyte>/<snap_lbyte>
                 *     snap_spec   :== 0 or 1 where 0 is Any and 1 is use snap_hbyte and snap_lbyte
                 *     snap_hbyte  :== 00..ff - value in hex (without 0x prepended)
                 *     snap_lbyte  :== 00..ff - value in hex (without 0x prepended)
                 *   type_ipv4     :== <proto>/<sip>/<ip_frag>/<dscp_low>/<dscp_high>[/<sport_low>/<sport_high>/<dport_low>/<dport_high>]
                 *     proto       :== -1 for Any, else value in decimal
                 *     sip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     ip_frag     :== 0: Any, 1: No, 2: Yes
                 *     dscp_low    :== -1..63 // -1 if Any
                 *     dscp_high   :== -1..63 // -1: Use dscp_low as specific, else high range.
                 *     sport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     sport_high  :== -1..65535 // -1: Use sport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *     dport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     dport_high  :== -1..65535 // -1: Use dport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *   type_ipv6     :== <proto>/<sip>/<dscp_low>/<dscp_high>[/<sport_low>/<sport_high>/<dport_low>/<dport_high>]
                 *     proto       :== -1 for Any, else value in decimal
                 *     sip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     dscp_low    :== -1..63 // -1 if Any
                 *     dscp_high   :== -1..63 // -1: Use dscp_low as specific, else high range.
                 *     sport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     sport_high  :== -1..65535 // -1: Use sport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *     dport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     dport_high  :== -1..65535 // -1: Use dport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 */
                cyg_httpd_write_chunked(",", 1);

                if (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_conf, 0) == VTSS_OK) {
                    if (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_next, 1) == VTSS_OK) {
                        next_qce_id = qce_next.id;
                    } else {
                        next_qce_id = 0;//if it is last entry
                    }
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%d/",
                                  (int)((qce_conf.isid == VTSS_ISID_GLOBAL) ? -1 : topo_isid2usid(qce_conf.isid)),
                                  (int)next_qce_id);
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    first = TRUE;
                    for (iport = VTSS_PORT_NO_START; iport < VTSS_FRONT_PORT_COUNT; iport++) {
                        if (!VTSS_PORT_BF_GET(qce_conf.port_list, iport)) { // if not checked
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), (first) ? "%u" : ",%u", iport + 1);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                            first = FALSE;
                        }
                    }
                    bit_val = QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits, QOS_QCE_VLAN_TAG);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/", bit_val);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    bit_val = QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits, QOS_QCE_VLAN_DEI);
                    u8 mask = qce_conf.key.pcp.mask;
                    int value = (mask == 0) ? -1 : qce_conf.key.pcp.value;
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%d/%d,%d/%d/%s/%d/%d/%d/%d;%u",
                                  (qce_conf.key.vid.in_range == TRUE) ?
                                  (qce_conf.key.vid.vr.r.low) :
                                  (qce_conf.key.vid.vr.v.mask == 0) ? -1 :
                                  (qce_conf.key.vid.vr.v.value),
                                  (qce_conf.key.vid.in_range == TRUE) ?
                                  (qce_conf.key.vid.vr.r.high) : -1,
                                  value,//pcp low value
                                  (mask == 0) ? -1 :
                                  (int )(value + ((mask == 0xFF) ? 0 : (mask == 0xFE) ? 1 : 3)),//pcp high
                                  (bit_val != 0) ? (bit_val - 1) : (-1),
                                  (qce_conf.key.smac.mask[0] == 0 &&
                                   qce_conf.key.smac.mask[1] == 0 &&
                                   qce_conf.key.smac.mask[2] == 0) ? "Any" : (misc_mac_txt(qce_conf.key.smac.value, str_buff2)),
                                  (QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits, QOS_QCE_DMAC_TYPE)),
                                  (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_PRIO) == 1) ?
                                  qce_conf.action.prio : -1,
                                  (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_DP) == 1) ?
                                  qce_conf.action.dp : -1,
                                  (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_DSCP) == 1) ?
                                  qce_conf.action.dscp : -1,
                                  qce_conf.type);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    switch (qce_conf.type) {
                    case VTSS_QCE_TYPE_ANY:
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", "Any");
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    case VTSS_QCE_TYPE_ETYPE://Format: /[Eternet Type]/[Ethernet value]
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%04x",
                                      (qce_conf.key.frame.etype.etype.mask[0] != 0 ||
                                       qce_conf.key.frame.etype.etype.mask[1] != 0) ? 1 : 0,
                                      qce_conf.key.frame.etype.etype.value[0] << 8 |
                                      qce_conf.key.frame.etype.etype.value[1]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    case VTSS_QCE_TYPE_LLC://Format: /[SSAP Value]/[DSAP Value]/[Control Value]
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%02x/%02x/%02x",
                                      (qce_conf.key.frame.llc.ssap.mask == 0) ? -1 : qce_conf.key.frame.llc.ssap.value,
                                      (qce_conf.key.frame.llc.dsap.mask == 0) ? -1 : qce_conf.key.frame.llc.dsap.value,
                                      (qce_conf.key.frame.llc.control.mask == 0) ? -1 : qce_conf.key.frame.llc.control.value,
                                      qce_conf.key.frame.llc.ssap.value,
                                      qce_conf.key.frame.llc.dsap.value,
                                      qce_conf.key.frame.llc.control.value);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    case VTSS_QCE_TYPE_SNAP://Format: [Eternet Type]/[Ethernet value 0]/[Ethernet value 1]
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%02x/%02x",
                                      (qce_conf.key.frame.snap.pid.mask[0] == 0 &&
                                       qce_conf.key.frame.snap.pid.mask[1] == 0) ? 0 : 1,
                                      qce_conf.key.frame.snap.pid.value[0],
                                      qce_conf.key.frame.snap.pid.value[1]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    case VTSS_QCE_TYPE_IPV4://Format: - [proto no]/[sip],[sip mask]/[ip fragment]/[dscp low]/[dscp high] -
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%s,%s/%d/%d/%d",
                                      (qce_conf.key.frame.ipv4.proto.mask == 0) ? -1 :
                                      qce_conf.key.frame.ipv4.proto.value,
                                      (qce_conf.key.frame.ipv4.sip.mask == 0) ? "Any" :
                                      misc_ipv4_txt(qce_conf.key.frame.ipv4.sip.value, str_buff1),
                                      misc_ipv4_txt(qce_conf.key.frame.ipv4.sip.mask, str_buff2),
                                      (QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits,
                                                              QOS_QCE_IPV4_FRAGMENT)),
                                      (qce_conf.key.frame.ipv4.dscp.in_range == TRUE) ?
                                      qce_conf.key.frame.ipv4.dscp.vr.r.low :
                                      (qce_conf.key.frame.ipv4.dscp.vr.v.mask == 0) ? -1 :
                                      qce_conf.key.frame.ipv4.dscp.vr.v.value,
                                      (qce_conf.key.frame.ipv4.dscp.in_range == TRUE) ?
                                      qce_conf.key.frame.ipv4.dscp.vr.r.high : -1);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        // format if TCP || UDP: /[sport low]/[sport high]/[dport low]/[dport high]
                        if (qce_conf.key.frame.ipv4.proto.value == 17 || qce_conf.key.frame.ipv4.proto.value == 6) { //UDP or TCP
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%d",
                                          (qce_conf.key.frame.ipv4.sport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv4.sport.vr.r.low :
                                          (qce_conf.key.frame.ipv4.sport.vr.v.mask == 0) ? -1 :
                                          qce_conf.key.frame.ipv4.sport.vr.v.value,
                                          (qce_conf.key.frame.ipv4.sport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv4.sport.vr.r.high : -1,
                                          (qce_conf.key.frame.ipv4.dport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv4.dport.vr.r.low :
                                          (qce_conf.key.frame.ipv4.dport.vr.v.mask == 0) ? -1 :
                                          qce_conf.key.frame.ipv4.dport.vr.v.value,
                                          (qce_conf.key.frame.ipv4.dport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv4.dport.vr.r.high : -1);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }
                        break;
                    case VTSS_QCE_TYPE_IPV6://Format: - /[proto no]/[sip - ?]/[dscp low]/[dscp high]
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%s/%u.%u.%u.%u/%d/%d",
                                      (qce_conf.key.frame.ipv6.proto.mask == 0) ? -1 :
                                      (qce_conf.key.frame.ipv6.proto.value),
                                      (qce_conf.key.frame.ipv6.sip.mask[0] == 0 &&
                                       qce_conf.key.frame.ipv6.sip.mask[1] == 0 &&
                                       qce_conf.key.frame.ipv6.sip.mask[2] == 0 &&
                                       qce_conf.key.frame.ipv6.sip.mask[3] == 0) ? "Any" :
                                      qcl_qce_ipv6_txt(&qce_conf.key.frame.ipv6.sip, buf),
                                      qce_conf.key.frame.ipv6.sip.mask[0],
                                      qce_conf.key.frame.ipv6.sip.mask[1],
                                      qce_conf.key.frame.ipv6.sip.mask[2],
                                      qce_conf.key.frame.ipv6.sip.mask[3],
                                      (qce_conf.key.frame.ipv6.dscp.in_range == TRUE) ?
                                      qce_conf.key.frame.ipv6.dscp.vr.r.low :
                                      (qce_conf.key.frame.ipv6.dscp.vr.v.mask == 0) ? -1 :
                                      qce_conf.key.frame.ipv6.dscp.vr.v.value,
                                      (qce_conf.key.frame.ipv6.dscp.in_range == TRUE) ?
                                      qce_conf.key.frame.ipv6.dscp.vr.r.high : -1);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        // format if TCP || UDP: /[sport low]/[sport high]/[dport low]/[dport high]
                        if (((qce_conf.key.frame.ipv6.proto.value) == 17) || ((qce_conf.key.frame.ipv6.proto.value) == 6)) { //UDP or TCP
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%d",
                                          (qce_conf.key.frame.ipv6.sport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv6.sport.vr.r.low :
                                          (qce_conf.key.frame.ipv6.sport.vr.v.mask == 0) ? -1 :
                                          qce_conf.key.frame.ipv6.sport.vr.v.value,
                                          (qce_conf.key.frame.ipv6.sport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv6.sport.vr.r.high : -1,
                                          (qce_conf.key.frame.ipv6.dport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv6.dport.vr.r.low :
                                          (qce_conf.key.frame.ipv6.dport.vr.v.mask == 0) ? -1 :
                                          qce_conf.key.frame.ipv6.dport.vr.v.value,
                                          (qce_conf.key.frame.ipv6.dport.in_range == TRUE) ?
                                          qce_conf.key.frame.ipv6.dport.vr.r.high : -1);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }
                        break;
                    default:
                        T_W("Invalid qce_conf.type field value");
                        break;
                    }
                }
                cyg_httpd_end_chunked();
                return -1; // return from edit case
            default:
                break;
            }
        }

        /* Format (for 'qcl_v2.htm' webpage only):
         * <usid_list>#<qce_list>
         *
         * usid_list :== <usid l>;<usid m>;<usid n> // List of present (active) switches in usid order.
         *
         * qce_list       :== <qce 1>;<qce 2>;<qce 3>;...<qce n> // List of currently defined QCEs (might be empty).
         *   qce  x       :== <usid>/<qcl_id>/<qce_id>/<frame_type>/<ports>/<key_smac>/<key_dmac>/<key_vid>/<key_pcp>/<key_dei>/<act_class>/<act_dpl>/<act_dscp>
         *     usid       :== 1..16 or -1 for all switches
         *     qcl_id     :== 1
         *     qce_id     :== 1..256
         *     frame_type :== 0..5    // One of vtss_qce_type_t
         *     ports      :== String  // List of ports e.g. "1,3,5,7,9-53"
         *     key_smac   :== String  // "Any" or "xx-xx-xx-xx-xx-xx"
         *     key_dmac   :== 0..3    // One of qos_qce_dmac_type_t
         *     key_vid    :== String  // "Any" or vid or vid-vid
         *     key_pcp    :== <pcp_low>,<pcp_high>
         *       pcp_low  :== -1..7   // -1 is Any
         *       pcp_high :== 0..7    // Not used if pcp_low == -1. If pcp_low != pcp_high then it is a range, else use pcp_low.
         *     key_dei    :== 0..2    // 0: Any, 1: 0, 2: 1
         *     act_class  :== -1..7   // -1 is no action, else classify to selected value
         *     act_dpl    :== -1..3   // -1 is no action, else classify to selected value
         *     act_dscp   :== -1..63  // -1 is no action, else classify to selected value
         */
        // Create a list of present switches in usid order separated by ";"
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
        while (switch_iter_getnext(&sit)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d", sit.first ? "" : ";", sit.usid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        // Create a list of currently defined QCEs
        first = TRUE;
        qce_id = QCE_ID_NONE;
        vtss_isid_t cur_sid = VTSS_ISID_GLOBAL;
        while (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_conf, 1) == VTSS_OK) {
            u8 mask = qce_conf.key.pcp.mask;
            int value = (mask == 0) ? -1 : qce_conf.key.pcp.value;
            qce_id = qce_conf.id;
            cur_sid = qce_conf.isid;
            (void) qcl_qce_port_list_txt(&qce_conf, buf); // Create port list
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%u/%u/%u/%s/%s/%d/%s/%d,%d/%u/%d/%d/%d",
                          (first) ? "#" : ";",
                          (cur_sid == VTSS_ISID_GLOBAL) ? -1 : topo_isid2usid(cur_sid),
                          qcl_id,
                          qce_conf.id,
                          qce_conf.type,
                          buf,
                          (qce_conf.key.smac.mask[0] == 0 && qce_conf.key.smac.mask[1] == 0 && qce_conf.key.smac.mask[2] == 0) ? "Any" : misc_mac_txt(qce_conf.key.smac.value, str_buff1),
                          QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits, QOS_QCE_DMAC_TYPE),
                          qcl_qce_range_txt_u16(&qce_conf.key.vid, str_buff2),
                          (qce_conf.key.pcp.mask != 0) ? qce_conf.key.pcp.value : -1,
                          (int )(value + ((mask == 0xFF) ? 0 : (mask == 0xFE) ? 1 : 3)),//pcp high
                          QCE_ENTRY_CONF_KEY_GET(qce_conf.key.key_bits, QOS_QCE_VLAN_DEI),
                          (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_PRIO) == 1) ? qce_conf.action.prio : -1,
                          (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_DP)   == 1) ? qce_conf.action.dp   : -1,
                          (QCE_ENTRY_CONF_ACTION_GET(qce_conf.action.action_bits, QOS_QCE_ACTION_DSCP) == 1) ? qce_conf.action.dscp : -1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            first = FALSE;
        }// end of while loop
        cyg_httpd_end_chunked();
    }// end of Get method response section
    return -1; // Do not further search the file system.
}// end of qce_handler function

/* support for qce status*/
static void qce_overview(CYG_HTTPD_STATE *p, int qcl_user, qos_qce_entry_conf_t *qce_conf)
{
    int ct;
    char buf[MGMT_PORT_BUF_SIZE];
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u/%d/%s/%d/%d/%d/%s",
                  qcl_user,
                  qce_conf->id,
                  qce_conf->type,
                  qcl_qce_port_list_txt(qce_conf, buf),
                  ((QCE_ENTRY_CONF_ACTION_GET(qce_conf->action.action_bits,
                                              QOS_QCE_ACTION_PRIO) == 1) ?
                   qce_conf->action.prio : -1),
                  ((QCE_ENTRY_CONF_ACTION_GET(qce_conf->action.action_bits,
                                              QOS_QCE_ACTION_DP) == 1) ?
                   qce_conf->action.dp : -1),
                  ((QCE_ENTRY_CONF_ACTION_GET(qce_conf->action.action_bits,
                                              QOS_QCE_ACTION_DSCP) == 1) ?
                   qce_conf->action.dscp : -1),
                  (qce_conf->conflict ? "Yes" : "No"));
    cyg_httpd_write_chunked(p->outbuffer, ct);
    cyg_httpd_write_chunked(";", 1);
}

static cyg_int32 handler_stat_qcl_v2(CYG_HTTPD_STATE *p)
{
    vtss_isid_t          isid;
    vtss_qce_id_t        qce_id;
    qos_qce_entry_conf_t qce_conf;
    int                  ct, qcl_user = -1, conflict, user_cnt;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        isid = web_retrieve_request_sid(p); /* Includes USID = ISID */

        if ((cyg_httpd_form_varable_int(p, "ConflictResolve", &conflict))) {
            if (conflict == 1) {
                (void)qos_mgmt_qce_conflict_resolve(isid, QCL_USER_STATIC, QCL_ID_END);
            }
        }
        if ((cyg_httpd_form_varable_int(p, "qclUser", &qcl_user))) {
        }

        /* Format for 'qcl_v2_stat.htm):
         * <qcl_info>|<qce_list>
         *
         * qcl_info       :== <sel_user>/<user 1>/<user 2>/...<user n>
         *   sel_user     :== -2: Show Conflict, -1: Show Combined, 0: Show Static, 1: Show Voice VLAN
         *   user x       :== 0..n    // List of defined users to show in user selector between "Combined" and "Conflict".
         *
         * qce_list       :== <qce 1>;<qce 2>;<qce 3>;...<qce n> // List of currently defined QCEs (might be empty).
         *   qce  x       :== <user>/<qce_id>/<frame_type>/<ports>/<act_class>/<act_dpl>/<act_dscp>/<conflict>
         _     user       :== 0..n    // One of the defined users
         *     qce_id     :== 1..256
         *     frame_type :== 0..5    // One of vtss_qce_type_t
         *     ports      :== String  // List of ports e.g. "1,3,5,7,9-53"
         *     act_class  :== -1..7   // -1 is no action, else classify to selected value
         *     act_dpl    :== -1..3   // -1 is no action, else classify to selected value
         *     act_dscp   :== -1..63  // -1 is no action, else classify to selected value
         *     conflict   :== String  // "Yes" or "No"
         */
        cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", qcl_user);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        for (user_cnt = QCL_USER_STATIC; user_cnt < QCL_USER_CNT; user_cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", user_cnt);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_write_chunked("|", 1);
        if (qcl_user < 0) { // Show Combined (-1) or Conflict (-2)
            for (user_cnt = QCL_USER_CNT - 1; user_cnt >= QCL_USER_STATIC; user_cnt--) {
                qce_id = QCE_ID_NONE;
                while (qos_mgmt_qce_entry_get(isid, user_cnt, QCL_ID_END, qce_id, &qce_conf, 1) == VTSS_OK) {
                    qce_id = qce_conf.id;
                    if (qcl_user == -1 || (qcl_user == -2 && qce_conf.conflict)) {
                        qce_overview(p, user_cnt, &qce_conf);
                    }
                }
            }
        } else { // Show selected user only (0..n)
            qce_id = QCE_ID_NONE;
            while (qos_mgmt_qce_entry_get(isid, qcl_user, QCL_ID_END, qce_id, &qce_conf, 1) == VTSS_OK) {
                qce_id = qce_conf.id;
                qce_overview(p, qcl_user, &qce_conf);
            }
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
#endif /* VTSS_FEATURE_QCL_V2 */
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
static cyg_int32 handler_config_dscp_port_config (CYG_HTTPD_STATE *p)
{
    vtss_isid_t     isid;
    port_iter_t     pit;
    qos_port_conf_t conf;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
            return -1;
        }
        int val, errors = 0;
        vtss_rc rc;
        char search_str[32];
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = qos_port_conf_get(isid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_get(%u, %d): failed rc = %d", isid, pit.uport, rc);
                continue;
            }
            sprintf(search_str, "enable_%d", pit.uport);
            /* Translate */
            conf.dscp_translate = FALSE;
            if (cyg_httpd_form_varable_find(p, search_str)) {/* "on" if checked */
                conf.dscp_translate = TRUE;
            }
            /* Classify */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "classify_%u", pit.uport)) {
                conf.dscp_imode = val;
            }
            /* Rewrite */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "rewrite_%u", pit.uport)) {
                switch (val) {
                case 0:/* Disable */
                case 1:/* Ensable */
                    conf.dscp_emode = val;
                    break;
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
                case 2:/* Remap DP Unaware */
                    conf.dscp_emode = VTSS_DSCP_EMODE_REMAP;
                    break;
                case 3:/* Remap DP aware */
                    conf.dscp_emode = VTSS_DSCP_EMODE_REMAP_DPA;
                    break;
#else
                case 2:/* Remap */
                    conf.dscp_emode = VTSS_DSCP_EMODE_REMAP;
                    break;
#endif
                default:
                    /* printf Error msg here */
                    break;
                }
            }
            (void) qos_port_conf_set(isid, pit.iport, &conf);
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_dscp_config.htm");
    } else {//GET Method
        /*Format:
         <port number>/<ingr. translate>/<ingr. classify>/<egr. rewrite>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(isid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u#%u#%u",
                          pit.first ? "" : "|",
                          pit.uport,
                          conf.dscp_translate,
                          conf.dscp_imode,
                          conf.dscp_emode);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
static cyg_int32 handler_config_dscp_based_qos_ingr_classi (CYG_HTTPD_STATE *p)
{
    qos_conf_t conf;
    //vtss_isid_t     isid;
    int             cnt;
    int val;
    int errors = 0;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (qos_conf_get(&conf) != VTSS_OK) {
        errors++;
        T_D("qos_conf_get(): failed no of errors = %d", errors);
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        char search_str[32];
        for (cnt = 0; cnt < 64; cnt++) {
            /* Trust trust_chk_ */
            sprintf(search_str, "trust_chk_%d", cnt);
            conf.dscp.dscp_trust[cnt] = FALSE;
            if (cyg_httpd_form_varable_find(p, search_str)) {/* "on" if checked */
                conf.dscp.dscp_trust[cnt] = TRUE;
            }
            /* Class classify_sel_ */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "classify_sel_%d", cnt)) {
                conf.dscp.dscp_qos_class_map[cnt] = val;
            }
            /* DPL dpl_sel_ */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dpl_sel_%d", cnt)) {
                conf.dscp.dscp_dp_level_map[cnt] = val;
            }
        }
        (void) qos_conf_set(&conf);
        redirect(p, errors ? STACK_ERR_URL : "/dscp_based_qos_ingr_classifi.htm");
    } else {/* GET Method */
        /*Format:
         <dscp number>#<trust>#<QoS class>#<dpl>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cnt = 0; cnt < 64; cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d#%d#%u#%d",
                          (cnt != 0) ? "|" : "",
                          cnt,
                          conf.dscp.dscp_trust[cnt],
                          conf.dscp.dscp_qos_class_map[cnt],
                          conf.dscp.dscp_dp_level_map[cnt]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
static cyg_int32 handler_config_dscp_translation (CYG_HTTPD_STATE *p)
{
    qos_conf_t      conf;
    int             cnt;
    int             temp;
    int errors = 0;
    char search_str[32];
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (qos_conf_get(&conf) != VTSS_OK) {
        errors++;
        T_D("qos_conf_get(): failed no of errors = %d", errors);
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (cnt = 0; cnt < 64; cnt++) {
            /*DSCP translate */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "trans_sel_%d", cnt)) {
                conf.dscp.translate_map[cnt] = temp;
            }
            /* classify */
            conf.dscp.ingress_remark[cnt] = FALSE;
            sprintf(search_str, "classi_chk_%d", cnt);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                conf.dscp.ingress_remark[cnt] = TRUE;
            }
            /* Remap DP0 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "rmp_dp0_%d", cnt)) {
                conf.dscp.egress_remap[cnt] = temp;
            }
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            /* Remap DP1 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "rmp_dp1_%d", cnt)) {
                conf.dscp.egress_remap_dp1[cnt] = temp;
            }
#endif
        }
        (void) qos_conf_set(&conf);
        redirect(p, errors ? STACK_ERR_URL : "/dscp_translation.htm");
    } else {/* GET method */
        /*Format
         <dscp number>/<DSCP translate>/<classify>/<remap DP0>/<remap DP1>|...
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cnt = 0; cnt < 64; cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%d/%d/%d",
                          (cnt != 0) ? "|" : "",
                          cnt,
                          conf.dscp.translate_map[cnt],
                          conf.dscp.ingress_remark[cnt],
                          conf.dscp.egress_remap[cnt]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                          conf.dscp.egress_remap_dp1[cnt]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
#endif
        }
        cyg_httpd_write_chunked("|", 1);
        cyg_httpd_end_chunked();
    }
    return -1;
}
static cyg_int32 handler_config_qos_dscp_classification_map (CYG_HTTPD_STATE *p)
{
    qos_conf_t      conf;
    int             cls_cnt, temp;
    int errors = 0;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (qos_conf_get(&conf) != VTSS_OK) {
        errors++;
        T_D("qos_conf_get(): failed no of errors = %d", errors);
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (cls_cnt = 0; cls_cnt < QOS_PORT_PRIO_CNT; cls_cnt++) {
            /* DSCP map to dp 0 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_0_%d", cls_cnt)) {
                conf.dscp.qos_class_dscp_map[cls_cnt] = temp;
            }
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            /* DSCP map to dp 1 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_1_%d", cls_cnt)) {
                conf.dscp.qos_class_dscp_map_dp1[cls_cnt] = temp;
            }
#endif
        }
        (void) qos_conf_set(&conf);
        redirect(p, errors ? STACK_ERR_URL : "/dscp_classification.htm");
    } else {
        /*Format:
         <dscp class>/<dscp 0>/<dscp 1>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cls_cnt = 0; cls_cnt < QOS_PORT_PRIO_CNT; cls_cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%d",
                          (cls_cnt != 0) ? "|" : "",
                          cls_cnt,
                          conf.dscp.qos_class_dscp_map[cls_cnt]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                          conf.dscp.qos_class_dscp_map_dp1[cls_cnt]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
#else
            cyg_httpd_write_chunked("/", 1);
#endif
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /*DSCP*/

#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
static cyg_int32 handler_config_qos_port_classification(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int val, errors = 0;
        vtss_rc rc;
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
        char search_str[32];
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = qos_port_conf_get(isid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_get(%u, %d): failed rc = %d", isid, pit.uport, rc);
                continue;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "class_%u", pit.uport)) {
                conf.default_prio = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dpl_%u", pit.uport)) {
                conf.default_dpl = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "pcp_%u", pit.uport)) {
                conf.usr_prio = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dei_%u", pit.uport)) {
                conf.default_dei = val;
            }
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
            sprintf(search_str, "dscp_enable_%d", pit.uport);
            conf.dscp_class_enable = FALSE;
            if (cyg_httpd_form_varable_find(p, search_str)) {/* "on" if checked */
                conf.dscp_class_enable = TRUE;
            }
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
            if ((rc = qos_port_conf_set(isid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", isid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_classification.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        int ct;
        cyg_httpd_start_chunked("html");

        /*
         * Format:
         * <options>|<ports>
         *
         * options :== <show_pcp_dei>,<show_tag_classification>,<show_dscp_classification>
         *   show_pcp_dei             :== 0..1 // 0: hide - , 1: show pcp and dei select cells
         *   show_tag_classification  :== 0..1 // 0: hide - , 1: show tag classification
         *   show_dscp_classification :== 0..1 // 0: hide - , 1: show dscp classification
         *
         * ports :== <port 1>,<port 2>,<port 3>,...<port n>
         *   port x :== <port_no>#<default_pcp>#<default_dei>#<default_class>#<volatile_class>#<default_dpl>#<tag_class>#<dscp_class>
         *     port_no        :== 1..max
         *     default_pcp    :== 0..7
         *     default_dei    :== 0..1
         *     default_class  :== 0..7
         *     volatile_class :== 0..7 or -1 if volatile is not set
         *     default_dpl    :== 0..3 on jaguar, 0..1 on luton26/Serval
         *     tag_class      :== 0..1 // 0: Disabled, 1: Enabled
         *     dscp_class     :== 0..1 // 0: Disabled, 1: Enabled
         */

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u|",
#if defined(QOS_USE_FIXED_PCP_QOS_MAP)
                      0, 0,
#else
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                      1, 1,
#else
                      1, 0,
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* defined(QOS_USE_FIXED_PCP_QOS_MAP) */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                      1
#else
                      0
#endif
                     );

        cyg_httpd_write_chunked(p->outbuffer, ct);
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_prio_t vol_prio = QOS_PORT_PRIO_UNDEF;

            if (qos_port_conf_get(isid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            (void) qos_port_volatile_get_default_prio(isid, pit.iport, &vol_prio);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u#%u#%u#%d#%u#%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.usr_prio,
                          conf.default_dei,
                          conf.default_prio,
                          vol_prio == QOS_PORT_PRIO_UNDEF ? -1 : vol_prio,
                          conf.default_dpl,
                          conf.tag_class_enable,
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                          conf.dscp_class_enable);
#else
                          1);
#endif
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static cyg_int32 handler_config_qos_port_classification_map(CYG_HTTPD_STATE *p)
{
    int             ct, i, uport;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t  iport = VTSS_PORT_NO_START;
    qos_port_conf_t conf;
    char            buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport > VTSS_PORT_NO_END) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || qos_port_conf_get(sid, iport, &conf) != VTSS_OK) {
            errors++;
        } else {
            int            val;
            vtss_rc        rc;

            if (cyg_httpd_form_varable_int(p, "tag_class", &val)) {
                conf.tag_class_enable = val;
            }
            for (i = 0; i < (VTSS_PCPS * 2); i++) {
                if (cyg_httpd_form_variable_int_fmt(p, &val, "default_class_%d", i)) {
                    conf.qos_class_map[i / 2][i % 2] = val;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "default_dpl_%d", i)) {
                    conf.dp_level_map[i / 2][i % 2] = val;
                }
            }

            if ((rc = qos_port_conf_set(sid, iport, &conf)) < 0) {
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_classification_map.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport > VTSS_PORT_NO_END) {
            iport = VTSS_PORT_NO_START;
        }

        if (qos_port_conf_get(sid, iport, &conf) == VTSS_OK) {

            /* Format:
             * <port_no>#<tag_class>#<map>
             *
             * port_no       :== 1..max
             * tag_class     :== 0..1 // 0: Disabled, 1: Enabled
             * map           :== <entry_0>/<entry_1>/...<entry_n> // n is 15.
             *   entry_x     :== <class|dpl>
             *     class     :== 0..7
             *     dpl       :== 0..3
             *
             * The map is organized as follows:
             * Entry corresponds to PCP,       DEI
             *  0                   0          0
             *  1                   0          1
             *  2                   1          0
             *  3                   1          1
             *  4                   2          0
             *  5                   2          1
             *  6                   3          0
             *  7                   3          1
             *  8                   4          0
             *  9                   4          1
             * 10                   5          0
             * 11                   5          1
             * 12                   6          0
             * 13                   6          1
             * 14                   7          0
             * 15                   7          1
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u",
                          iport2uport(iport),
                          conf.tag_class_enable);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (i = 0; i < (VTSS_PCPS * 2); i++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (i != 0) ? "/" : "#",
                              conf.qos_class_map[i / 2][i % 2],
                              conf.dp_level_map[i / 2][i % 2]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

/* Support for a given feature is encoded like this:
   0: No supported
   1: Supported on other ports
   2: Supported on this port */
static u8 port_feature_support(port_cap_t cap, port_cap_t port_cap, port_cap_t mask)
{
    return ((cap & mask) ? ((port_cap & mask) ? 2 : 1) : 0);
}

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL) && defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
static cyg_int32 handler_config_qos_port_policers_multi(CYG_HTTPD_STATE *p)
{
    int             ct;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_policers_multi.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<policers>
             *   port_no  :== 1..max
             *   policers :== <policer 1>/<policer 2>/<policer 3>/...<policer n>
             *     policer x :== <enabled>|<fps>|<rate>
             *       enabled :== 0..1           // 0: no, 1: yes
             *       fps     :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *       rate    :== 0..0xffffffff  // actual bit or frame rate
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%lu#%u|%u|%lu/%u|%u|%lu/%u|%u|%lu/%u|%u|%lu,",
                          pit.uport,
                          conf.port_policer[0].enabled,
                          conf.port_policer_ext[0].frame_rate,
                          conf.port_policer[0].policer.rate,
                          conf.port_policer[1].enabled,
                          conf.port_policer_ext[1].frame_rate,
                          conf.port_policer[1].policer.rate,
                          conf.port_policer[2].enabled,
                          conf.port_policer_ext[2].frame_rate,
                          conf.port_policer[2].policer.rate,
                          conf.port_policer[3].enabled,
                          conf.port_policer_ext[3].frame_rate,
                          conf.port_policer[3].policer.rate);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qos_port_policer_edit_multi(CYG_HTTPD_STATE *p)
{
    int             ct, policer, uport;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t  iport = VTSS_PORT_NO_START;
    qos_port_conf_t conf;
    char            buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport > VTSS_PORT_NO_END) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || qos_port_conf_get(sid, iport, &conf) != VTSS_OK) {
            errors++;
        } else {
            vtss_bitrate_t rate;
            int            val;
            vtss_rc        rc;

            for (policer = 0; policer < QOS_PORT_POLICER_CNT; policer++) {
                conf.port_policer[policer].enabled = cyg_httpd_form_variable_check_fmt(p, "enabled_%d", policer);

                if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "rate_%u", policer)) {
                    conf.port_policer[policer].policer.rate = rate;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &val, "fps_%d", policer)) {
                    conf.port_policer_ext[policer].frame_rate = (val != 0);
                }

                if (cyg_httpd_form_variable_int_fmt(p, &val, "dp_bypass_level_%d", policer)) {
                    conf.port_policer_ext[policer].dp_bypass_level = val;
                }

                conf.port_policer_ext[policer].unicast      = cyg_httpd_form_variable_check_fmt(p, "unicast_%d",      policer);
                conf.port_policer_ext[policer].multicast    = cyg_httpd_form_variable_check_fmt(p, "multicast_%d",    policer);
                conf.port_policer_ext[policer].broadcast    = cyg_httpd_form_variable_check_fmt(p, "broadcast_%d",    policer);
                conf.port_policer_ext[policer].flooded      = cyg_httpd_form_variable_check_fmt(p, "flooding_%d",     policer);
                conf.port_policer_ext[policer].learning     = cyg_httpd_form_variable_check_fmt(p, "learning_%d",     policer);
                conf.port_policer_ext[policer].flow_control = cyg_httpd_form_variable_check_fmt(p, "flow_control_%d", policer);
            }

            if ((rc = qos_port_conf_set(sid, iport, &conf)) < 0) {
                T_D("qos_port_conf_set(%u, %ld): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }


        (void)snprintf(buf, sizeof(buf), "/qos_port_policer_edit_multi.htm?port=%lu", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        port_status_t    port_status;
        port_isid_info_t info;
        port_cap_t       cap = (port_isid_info_get(sid, &info) == VTSS_RC_OK ? info.cap : 0);
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport > VTSS_PORT_NO_END) {
            iport = VTSS_PORT_NO_START;
        }

        if ((qos_port_conf_get(sid, iport, &conf) == VTSS_OK) &&
            (port_mgmt_status_get_all(sid, iport, &port_status) == VTSS_OK)) {

            /* Format:
             * <port_no>#<fc_mode>#<policers>
             *
             * port_no  :== 1..max
             * fc_mode  :== 0..2                      // 0: No ports has fc (don't show fc column), 1: This port has no fc, 2: This port has fc
             * policers :== <policer 1>/<policer 2>/<policer 3>/...<policer n>
             *   policer x         :== <enabled>|<fps>|<rate>|<dp_bypass_level>|<unicast>|<multicast>|<broadcast>|<flooded>|<learning>|<flow_control>
             *     enabled         :== 0..1           // 0: no, 1: yes
             *     fps             :== 0..1           // 0: unit for rate is kbits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *     rate            :== 0..0xffffffff  // actual bit or frame rate
             *     dp_bypass_level :== 0..3           // drop precedence bypass level
             *     unicast         :== 0..1           // unicast frames are policed
             *     multicast,      :== 0..1           // multicast frames are policed
             *     brooadcast      :== 0..1           // broadcast frames are policed
             *     flooded         :== 0..1           // flooded frames are policed
             *     learning        :== 0..1           // learning frames are policed
             *     flow_control    :== 0..1           // flow control is enabled
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%lu#%u#", iport2uport(iport), port_feature_support(cap, port_status.cap, PORT_CAP_FLOW_CTRL));
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (policer = 0; policer < QOS_PORT_POLICER_CNT; policer++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u|%lu|%u|%u|%u|%u|%u|%u|%u",
                              (policer != 0) ? "/" : "",
                              conf.port_policer[policer].enabled,
                              conf.port_policer_ext[policer].frame_rate,
                              conf.port_policer[policer].policer.rate,
                              conf.port_policer_ext[policer].dp_bypass_level,
                              conf.port_policer_ext[policer].unicast,
                              conf.port_policer_ext[policer].multicast,
                              conf.port_policer_ext[policer].broadcast,
                              conf.port_policer_ext[policer].flooded,
                              conf.port_policer_ext[policer].learning,
                              conf.port_policer_ext[policer].flow_control);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#else
static cyg_int32 handler_config_qos_port_policers(CYG_HTTPD_STATE *p)
{
    int             ct;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_rc        rc;
            vtss_bitrate_t rate;
            int            val;
            if ((rc = qos_port_conf_get(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_get(%u, %d): failed rc = %d", sid, pit.uport, rc);
                continue;
            }
            conf.port_policer[0].enabled = cyg_httpd_form_variable_check_fmt(p, "enabled_%u", pit.iport);

            if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "rate_%u", pit.iport)) {
                conf.port_policer[0].policer.rate = rate;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "fps_%u", pit.iport)) {
                conf.port_policer_ext[0].frame_rate = (val != 0);
            }

            conf.port_policer_ext[0].flow_control = cyg_httpd_form_variable_check_fmt(p, "flow_control_%u", pit.iport);

            if ((rc = qos_port_conf_set(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_policers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        port_status_t    port_status;
        port_isid_info_t info;
        port_cap_t       cap = (port_isid_info_get(sid, &info) == VTSS_RC_OK ? info.cap : 0);
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((qos_port_conf_get(sid, pit.iport, &conf) < 0) ||
                (port_mgmt_status_get_all(sid, pit.iport, &port_status) < 0)) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>/<enabled>/<fps>/<rate>/<flow_control>
             *   port_no      :== 1..max
             *   enabled      :== 0..1           // 0: no, 1: yes
             *   fps          :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *   rate         :== 0..0xffffffff  // actual bit or frame rate
             *   fc_mode      :== 0..2           // 0: No ports has fc (don't show fc column), 1: This port has no fc, 2: This port has fc
             *   flow_control :== 0..1           // flow control is enabled
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u/%u/%u/%u/%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.port_policer[0].enabled,
                          conf.port_policer_ext[0].frame_rate,
                          conf.port_policer[0].policer.rate,
                          port_feature_support(cap, port_status.cap, PORT_CAP_FLOW_CTRL),
                          conf.port_policer_ext[0].flow_control);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL) && defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM) */
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS) */

#if defined(VTSS_SW_OPTION_BUILD_CE)
static cyg_int32 handler_config_qos_queue_policers(CYG_HTTPD_STATE *p)
{
    int             ct, queue;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_rc        rc;
            vtss_bitrate_t rate;
            if ((rc = qos_port_conf_get(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_get(%u, %d): failed rc = %d", sid, pit.uport, rc);
                continue;
            }
            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                conf.queue_policer[queue].enabled = cyg_httpd_form_variable_check_fmt(p, "enabled_%d_%u", queue, pit.iport);

                if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "rate_%d_%u", queue, pit.iport)) {
                    conf.queue_policer[queue].policer.rate = rate;
                }
            }

            if ((rc = qos_port_conf_set(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_queue_policers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<queues>
             *   port_no :== 1..max
             *   queues  :== <queue 0>/<queue 1>/<queue 2>/...<queue n>
             *     queue x :== <enabled>|<rate>
             *       enabled :== 0..1           // 0: no, 1: yes
             *       rate    :== 0..0xffffffff  // bit rate
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                          pit.first ? "" : ",",
                          pit.uport);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_policer[queue].enabled,
                              conf.queue_policer[queue].policer.rate);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
static cyg_int32 handler_config_qos_port_schedulers(CYG_HTTPD_STATE *p)
{
    int             ct, queue;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_schedulers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<scheduler_mode>#<queue_weights>
             *   port_no          :== 1..max
             *   scheduler_mode   :== 0..1           // 0: Strict Priority, 1: Weighted
             *   queue_weights    :== <queue_1_weight>/<queue_2_weight>/...<queue_n_weight>  // n is 6.
             *     queue_x_weight :== 1..100         // Just a number. If you set all 6 weights to 100, each queue will have a weigth of 100/6 = 16.7 ~ 17%
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.dwrr_enable);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE - 2); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_pct[queue]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qos_port_shapers(CYG_HTTPD_STATE *p)
{
    int             ct, queue;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_shapers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x          :== <port_no>#<queue_shapers>#<port_shaper>
             *   port_no       :== 1..max
             *   queue_shapers :== <shaper_1>/<shaper_2>/...<shaper_n>                 // n is 8.
             *     shaper_x    :== <enable|rate>
             *       enable    :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *       rate      :== 0..0xffffffff  // Actual bit rate in kbps
             *   port_shaper   :== <enable|rate>
             *     enable      :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *     rate        :== 0..0xffffffff  // Actual bit rate in kbps
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                          pit.first ? "" : ",",
                          pit.uport);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_shaper[queue].enable,
                              conf.queue_shaper[queue].rate);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u|%u",
                          conf.shaper_status,
                          conf.shaper_rate);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qos_port_scheduler_edit(CYG_HTTPD_STATE *p)
{
    int             ct, queue, uport;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t  iport = VTSS_PORT_NO_START;
    qos_port_conf_t conf;
    char            buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport > VTSS_PORT_NO_END) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || qos_port_conf_get(sid, iport, &conf) != VTSS_OK) {
            errors++;
        } else {
            vtss_rc        rc;
            int            val;
            vtss_bitrate_t rate = 0;

            if (cyg_httpd_form_varable_int(p, "dwrr_enable", &val)) {
                conf.dwrr_enable = val;
            }

            // Get Port Scheduler configuration if dwrr is enabled
            if (conf.dwrr_enable) {
                for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE - 2); queue++) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "weight_%d", queue)) {
                        conf.queue_pct[queue] = val;
                    }
                }
            }

            // Get Queue Shaper configuration
            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                conf.queue_shaper[queue].enable = cyg_httpd_form_variable_check_fmt(p, "q_shaper_enable_%d", queue);

                // Only save rate and excess if shaper is enabled
                if (conf.queue_shaper[queue].enable) {
                    if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "q_shaper_rate_%d", queue)) {
                        conf.queue_shaper[queue].rate = rate;
                    }
                    conf.excess_enable[queue] = cyg_httpd_form_variable_check_fmt(p, "q_shaper_excess_%d", queue);
                }
            }

            // Get Port Shaper configuration
            conf.shaper_status = cyg_httpd_form_varable_find(p, "p_shaper_enable") ? TRUE : FALSE;

            // Only save rate if shaper is enabled
            if (conf.shaper_status && cyg_httpd_form_varable_long_int(p, "p_shaper_rate", &rate)) {
                conf.shaper_rate = rate;
            }

            if ((rc = qos_port_conf_set(sid, iport, &conf)) < 0) {
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_scheduler_edit.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport > VTSS_PORT_NO_END) {
            iport = VTSS_PORT_NO_START;
        }

        if (qos_port_conf_get(sid, iport, &conf) == VTSS_OK) {

            /* Format:
             * <port_no>#<scheduler_mode>#<queue_weights>#<queue_shapers>#<port_shaper>
             *
             * port_no            :== 1..max
             * scheduler_mode     :== 0..1           // 0: Strict Priority, 1: Weighted
             * queue_weights      :== <queue_1_weight>/<queue_2_weight>/...<queue_n_weight>  // n is 6.
             *   queue_x_weight   :== 1..100         // Just a number. If you set all 6 weights to 100, each queue will have a weigth of 100/6 = 16.7 ~ 17%
             * queue_shapers      :== <queue_shaper_1>/<queue_shaper_2>/...<queue_shaper_n>  // n is 8.
             *   queue_shaper_x   :== <enable|rate|excess>
             *     enable         :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *     rate           :== 0..0xffffffff  // Actual bit rate in kbps
             *     excess         :== 0..1           // 0: Excess bandwidth disabled, 1: Excess bandwidth enabled
             * port_shaper        :== <enable|rate>
             *   enable           :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *   rate             :== 0..0xffffffff  // Actual bit rate in kbps
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u",
                          iport2uport(iport),
                          conf.dwrr_enable);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE - 2); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_pct[queue]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u|%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_shaper[queue].enable,
                              conf.queue_shaper[queue].rate,
                              conf.excess_enable[queue]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u|%u",
                          conf.shaper_status,
                          conf.shaper_rate);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static cyg_int32 handler_config_qos_port_tag_remarking(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_tag_remarking.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        port_iter_t     pit;
        qos_port_conf_t conf;
        int             ct, mode;
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<mode>
             *   port_no       :== 1..max
             *   mode          :== 0..2              // 0: Classified, 1: Default, 2: Mapped
             */

            switch (conf.tag_remark_mode) {
            case VTSS_TAG_REMARK_MODE_DEFAULT:
                mode = 1;
                break;
            case VTSS_TAG_REMARK_MODE_MAPPED:
                mode = 2;
                break;
            default:
                mode = 0;
                break;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          mode);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qos_port_tag_remarking_edit(CYG_HTTPD_STATE *p)
{
    int             ct, i, uport;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t  iport = VTSS_PORT_NO_START;
    qos_port_conf_t conf;
    char            buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport > VTSS_PORT_NO_END) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || qos_port_conf_get(sid, iport, &conf) != VTSS_OK) {
            errors++;
        } else {
            int            val;
            vtss_rc        rc;

            if (cyg_httpd_form_varable_int(p, "tr_mode", &val)) {
                switch (val) {
                case 1:
                    conf.tag_remark_mode = VTSS_TAG_REMARK_MODE_DEFAULT;
                    break;
                case 2:
                    conf.tag_remark_mode = VTSS_TAG_REMARK_MODE_MAPPED;
                    break;
                default:
                    conf.tag_remark_mode = VTSS_TAG_REMARK_MODE_CLASSIFIED;
                    break;
                }
            }

            // Get default PCP and DEI if mode is set to "Default"
            if (conf.tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT) {
                if (cyg_httpd_form_varable_int(p, "default_pcp", &val)) {
                    conf.tag_default_pcp = val;
                }
                if (cyg_httpd_form_varable_int(p, "default_dei", &val)) {
                    conf.tag_default_dei = val;
                }
            }

            // Get DP level and Map if mode is set to "Mapped"
            if (conf.tag_remark_mode == VTSS_TAG_REMARK_MODE_MAPPED) {
                for (i = 0; i < QOS_PORT_TR_DPL_CNT; i++) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "dpl_%d", i)) {
                        conf.tag_dp_map[i] = val;
                    }
                }

                for (i = 0; i < (QOS_PORT_PRIO_CNT * 2); i++) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "pcp_%d", i)) {
                        conf.tag_pcp_map[i / 2][i % 2] = val;
                    }
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "dei_%d", i)) {
                        conf.tag_dei_map[i / 2][i % 2] = val;
                    }
                }
            }

            if ((rc = qos_port_conf_set(sid, iport, &conf)) < 0) {
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_tag_remarking_edit.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport > VTSS_PORT_NO_END) {
            iport = VTSS_PORT_NO_START;
        }

        if (qos_port_conf_get(sid, iport, &conf) == VTSS_OK) {

            /* Format:
             * <port_no>#<mode>#<default_params>#<dpl>#<map>
             *
             * port_no          :== 1..max
             * mode             :== 0..2       // 0: Classified, 1: Default, 2: Mapped
             * default_params   :== <pcp|dei>
             *   pcp            :== 0..7
             *   dei            :== 0..1
             * dpl              :== <dpl_0>/<dpl_1>/<dpl_2>/<dpl_3>
             *   dpl_x          :== 0..1
             * map              :== <entry_0>/<entry_1>/...<entry_n> // n is 15.
             *   entry_x        :== <pcp|dei>
             *     pcp          :== 0..7
             *     dei          :== 0..1
             *
             * The map is organized as follows:
             * Entry corresponds to QoS class, DP level
             *  0                   0          0
             *  1                   0          1
             *  2                   1          0
             *  3                   1          1
             *  4                   2          0
             *  5                   2          1
             *  6                   3          0
             *  7                   3          1
             *  8                   4          0
             *  9                   4          1
             * 10                   5          0
             * 11                   5          1
             * 12                   6          0
             * 13                   6          1
             * 14                   7          0
             * 15                   7          1
             */
            int mode;
            switch (conf.tag_remark_mode) {
            case VTSS_TAG_REMARK_MODE_DEFAULT:
                mode = 1;
                break;
            case VTSS_TAG_REMARK_MODE_MAPPED:
                mode = 2;
                break;
            default:
                mode = 0;
                break;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u#%u|%u",
                          iport2uport(iport),
                          mode,
                          conf.tag_default_pcp,
                          conf.tag_default_dei);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (i = 0; i < QOS_PORT_TR_DPL_CNT; i++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                              (i != 0) ? "/" : "#",
                              conf.tag_dp_map[i]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            for (i = 0; i < (QOS_PORT_PRIO_CNT * 2); i++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (i != 0) ? "/" : "#",
                              conf.tag_pcp_map[i / 2][i % 2],
                              conf.tag_dei_map[i / 2][i % 2]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
static cyg_int32 handler_config_stormctrl_jr(CYG_HTTPD_STATE *p)
{
    int             ct;
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_rc        rc;
            vtss_bitrate_t rate;
            int            val;
            if ((rc = qos_port_conf_get(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_get(%u, %d): failed rc = %d", sid, pit.uport, rc);
                continue;
            }
            conf.port_policer[QOS_STORM_POLICER_UNICAST].enabled = cyg_httpd_form_variable_check_fmt(p, "uc_enabled_%u", pit.iport);

            if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "uc_rate_%u", pit.iport)) {
                conf.port_policer[QOS_STORM_POLICER_UNICAST].policer.rate = rate;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "uc_fps_%u", pit.iport)) {
                conf.port_policer_ext[QOS_STORM_POLICER_UNICAST].frame_rate = (val != 0);
            }

            conf.port_policer[QOS_STORM_POLICER_BROADCAST].enabled = cyg_httpd_form_variable_check_fmt(p, "bc_enabled_%u", pit.iport);

            if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "bc_rate_%u", pit.iport)) {
                conf.port_policer[QOS_STORM_POLICER_BROADCAST].policer.rate = rate;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "bc_fps_%u", pit.iport)) {
                conf.port_policer_ext[QOS_STORM_POLICER_BROADCAST].frame_rate = (val != 0);
            }

            conf.port_policer[QOS_STORM_POLICER_UNKNOWN].enabled = cyg_httpd_form_variable_check_fmt(p, "un_enabled_%u", pit.iport);

            if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "un_rate_%u", pit.iport)) {
                conf.port_policer[QOS_STORM_POLICER_UNKNOWN].policer.rate = rate;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "un_fps_%u", pit.iport)) {
                conf.port_policer_ext[QOS_STORM_POLICER_UNKNOWN].frame_rate = (val != 0);
            }

            if ((rc = qos_port_conf_set(sid, pit.iport, &conf)) < 0) {
                errors++; /* Probably stack error */
                T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/stormctrl_jr.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>/<unicast>/<broadcast>/<unknown>
             *   port_no   :== 1..max
             *   unicast   :== <enabled>|<fps>|<rate>
             *     enabled :== 0..1           // 0: no, 1: yes
             *     fps     :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *     rate    :== 0..0xffffffff  // actual bit or frame rate
             *   broadcast :== <enabled>|<fps>|<rate>
             *     enabled :== 0..1           // 0: no, 1: yes
             *     fps     :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *     rate    :== 0..0xffffffff  // actual bit or frame rate
             *   unknown   :== <enabled>|<fps>|<rate>
             *     enabled :== 0..1           // 0: no, 1: yes
             *     fps     :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *     rate    :== 0..0xffffffff  // actual bit or frame rate
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u|%u|%u/%u|%u|%u/%u|%u|%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.port_policer[QOS_STORM_POLICER_UNICAST].enabled,
                          conf.port_policer_ext[QOS_STORM_POLICER_UNICAST].frame_rate,
                          conf.port_policer[QOS_STORM_POLICER_UNICAST].policer.rate,
                          conf.port_policer[QOS_STORM_POLICER_BROADCAST].enabled,
                          conf.port_policer_ext[QOS_STORM_POLICER_BROADCAST].frame_rate,
                          conf.port_policer[QOS_STORM_POLICER_BROADCAST].policer.rate,
                          conf.port_policer[QOS_STORM_POLICER_UNKNOWN].enabled,
                          conf.port_policer_ext[QOS_STORM_POLICER_UNKNOWN].frame_rate,
                          conf.port_policer[QOS_STORM_POLICER_UNKNOWN].policer.rate);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#ifdef VTSS_FEATURE_QOS_WRED
static cyg_int32 handler_config_qos_wred(CYG_HTTPD_STATE *p)
{
    int          ct, queue;
    qos_conf_t   conf, newconf;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (qos_conf_get(&conf) != VTSS_OK) {
            errors++;   /* Probably stack error */
        } else {
            newconf = conf;

            for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                int  val;
                char var_enable[32];
                sprintf(var_enable, "enable_%d", queue);
                newconf.wred[queue].enable = FALSE;
                if (cyg_httpd_form_varable_find(p, var_enable)) { /* enabled if present */
                    newconf.wred[queue].enable = TRUE;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "min_th_%d", queue)) {
                    newconf.wred[queue].min_th = val;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "mdp_1_%d", queue)) {
                    newconf.wred[queue].max_prob_1 = val;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "mdp_2_%d", queue)) {
                    newconf.wred[queue].max_prob_2 = val;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "mdp_3_%d", queue)) {
                    newconf.wred[queue].max_prob_3 = val;
                }
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                vtss_rc rc;
                if ((rc = qos_conf_set(&newconf)) != VTSS_OK) {
                    T_D("qos_conf_set: failed rc = %d", rc);
                    errors++; /* Probably stack error */
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_wred.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        /* should get these values from management APIs */
        if (qos_conf_get(&conf) == VTSS_OK) {
            /*
             * Format:
             * <queue 0>,<queue 1>,<queue 2>,...<queue n> // n is 5.
             *
             * queue x :== <enable>#<min_th>#<mdp_1>#<mdp_2>#<mdp_3>
             *   enable :== 0..1
             *   min_th :== 0..100
             *   mdp_1  :== 0..100
             *   mdp_2  :== 0..100
             *   mdp_3  :== 0..100
             */
            for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d#%u#%u#%u#%u",
                              (queue != 0) ? "," : "",
                              conf.wred[queue].enable,
                              conf.wred[queue].min_th,
                              conf.wred[queue].max_prob_1,
                              conf.wred[queue].max_prob_2,
                              conf.wred[queue].max_prob_3);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_FEATURE_QOS_WRED */

#ifdef VTSS_FEATURE_VSTAX_V2
static cyg_int32 handler_config_qos_cmef(CYG_HTTPD_STATE *p)
{
    int          ct;
    qos_conf_t   conf;
    BOOL         enabled;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (qos_conf_get(&conf) != VTSS_OK) {
            errors++;   /* Probably stack error */
        } else {
            enabled = (cyg_httpd_form_varable_find(p, "cmef_mode") != NULL);
            if (enabled == conf.cmef_disable) { /* NOTE: Different polarity!!! */
                vtss_rc rc;
                conf.cmef_disable = !enabled;
                if ((rc = qos_conf_set(&conf)) != VTSS_OK) {
                    T_D("qos_conf_set: failed rc = %d", rc);
                    errors++; /* Probably stack error */
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_cmef.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (qos_conf_get(&conf) == VTSS_OK) {
            /*
             * Format:
             * cmef_mode :== 0..1 // 0: Disabled, 1: Enabled
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", !conf.cmef_disable);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

/****************************************************************************/
/*  LUTON28 code starts here                                                */
/****************************************************************************/
#elif defined(VTSS_ARCH_LUTON28)

static cyg_int32 handler_config_qos_class(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_conf_t      qos_conf;
    qos_port_conf_t qos_port_conf;
    int             ct;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        char var_rate[32];
        int q_no, class_id, qcl_id;
        int usr_prio;
        int wfq, q_weight1, q_weight2, q_weight3, q_weight4;

        if (qos_conf_get(&qos_conf) < 0) {
            errors++; /* Probably stack error - bail out */
        }

        if (cyg_httpd_form_varable_int(p, "cselect", &q_no)) {
            if (q_no != qos_conf.prio_no) {
                vtss_rc rc;
                qos_conf.prio_no = q_no;
                if ((rc = qos_conf_set(&qos_conf)) < 0) {
                    T_D("qos_conf_set: failed rc = %d", rc);
                    errors++;         /* Probably stack error */
                }
            }
        }

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &qos_port_conf) < 0) {
                errors++;   /* Probably stack error */
                T_D("qos_port_conf_get(%u,%d): failed ", sid, pit.uport);
                continue;
            }
            sprintf(var_rate, "class_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_rate, &class_id)) {
                sprintf(var_rate, "qcl_%d", pit.uport);
                if (cyg_httpd_form_varable_int(p, var_rate, &qcl_id)) {
                    sprintf(var_rate, "user_prio_%d", pit.uport);
                    if (cyg_httpd_form_varable_int(p, var_rate, &usr_prio)) {
                        sprintf(var_rate, "queue_mode_%d", pit.uport);
                        if (cyg_httpd_form_varable_int(p, var_rate, &wfq)) {
                            sprintf(var_rate, "low_queue_mode_%d", pit.uport);
                            if (!cyg_httpd_form_varable_int(p, var_rate, &q_weight1)) {
                                q_weight1 = qos_port_conf.weight[VTSS_QUEUE_START];
                            }
                            sprintf(var_rate, "normal_queue_mode_%d", pit.uport);
                            if (!cyg_httpd_form_varable_int(p, var_rate, &q_weight2)) {
                                q_weight2 = qos_port_conf.weight[VTSS_QUEUE_START + 1];
                            }
                            sprintf(var_rate, "medium_queue_mode_%d", pit.uport);
                            if ( !cyg_httpd_form_varable_int(p, var_rate, &q_weight3)) {
                                q_weight3 = qos_port_conf.weight[VTSS_QUEUE_START + 2];
                            }
                            sprintf(var_rate, "high_queue_mode_%d", pit.uport);
                            if (!cyg_httpd_form_varable_int(p, var_rate, &q_weight4)) {
                                q_weight4 = qos_port_conf.weight[VTSS_QUEUE_START + 3];
                            }
                            class_id = uprio2iprio(class_id);
                            if ( class_id != qos_port_conf.default_prio               ||
                                 qcl_id != qos_port_conf.qcl_no                     ||
                                 usr_prio != qos_port_conf.usr_prio                   ||
                                 wfq != qos_port_conf.wfq_enable                 ||
                                 q_weight1 != qos_port_conf.weight[VTSS_QUEUE_START]   ||
                                 q_weight2 != qos_port_conf.weight[VTSS_QUEUE_START + 1] ||
                                 q_weight3 != qos_port_conf.weight[VTSS_QUEUE_START + 2] ||
                                 q_weight4 != qos_port_conf.weight[VTSS_QUEUE_START + 3]) {
                                vtss_rc rc;
                                qos_port_conf.default_prio                 = class_id;
                                qos_port_conf.qcl_no                       = qcl_id;
                                qos_port_conf.usr_prio                     = usr_prio;
                                qos_port_conf.wfq_enable                   = wfq;
                                qos_port_conf.weight[VTSS_QUEUE_START + 0] = q_weight1;
                                qos_port_conf.weight[VTSS_QUEUE_START + 1] = q_weight2;
                                qos_port_conf.weight[VTSS_QUEUE_START + 2] = q_weight3;
                                qos_port_conf.weight[VTSS_QUEUE_START + 3] = q_weight4;
                                if ((rc = qos_port_conf_set(sid, pit.iport, &qos_port_conf)) < 0) {
                                    T_D("qos_port_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
                                    errors++; /* Probably stack error */
                                }
                            }
                        }
                    }
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_class.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        if (qos_conf_get(&qos_conf) < 0) {
            cyg_httpd_end_chunked();
            return -1;          /* Probably stack error - bail out */
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", qos_conf.prio_no);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &qos_port_conf) < 0) {
                break;    /* Probably stack error - bail out */
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u/%u/%u|",
                          pit.uport,
                          iprio2uprio(qos_port_conf.default_prio),
                          qos_port_conf.qcl_no,
                          qos_port_conf.usr_prio,
                          qos_port_conf.wfq_enable,
                          qos_port_conf.weight[VTSS_QUEUE_START + 0],
                          qos_port_conf.weight[VTSS_QUEUE_START + 1],
                          qos_port_conf.weight[VTSS_QUEUE_START + 2],
                          qos_port_conf.weight[VTSS_QUEUE_START + 3]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#ifdef VTSS_SW_OPTION_BUILD_SMB
static cyg_int32 handler_config_dscp_remarking(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t qos_port_conf;
    int             ct;
    vtss_rc         rc;
    int             var_value;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        char var_str[32];

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &qos_port_conf) < 0) {
                errors++;   /* Probably stack error */
                T_D("qos_port_conf_get(%u,%d): failed ", sid, pit.uport);
                continue;
            }
            sprintf(var_str, "mode_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_str, &var_value)) {
                qos_port_conf.qos_remarking_enable = var_value;
                sprintf(var_str, "low_queue_mapping_%d", pit.uport);
                if (cyg_httpd_form_varable_int(p, var_str, &var_value)) {
                    qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START] = var_value;
                    sprintf(var_str, "normal_queue_mapping_%d", pit.uport);
                    if (cyg_httpd_form_varable_int(p, var_str, &var_value) ) {
                        qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 1] = var_value;
                        sprintf(var_str, "medium_queue_mapping_%d", pit.uport);
                        if (cyg_httpd_form_varable_int(p, var_str, &var_value)) {
                            qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 2] = var_value;
                            sprintf(var_str, "high_queue_mapping_%d", pit.uport);
                            if (cyg_httpd_form_varable_int(p, var_str, &var_value)) {
                                qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 3] = var_value;
                                if ((rc = qos_port_conf_set(sid, pit.iport, &qos_port_conf)) < 0) {
                                    T_D("qos_port_conf_set(%u,%d): failed rc = %d", sid, pit.uport, rc);
                                    errors++; /* Probably stack error */
                                }
                            }
                        }
                    }
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/dscp_remarking.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &qos_port_conf) < 0) {
                break;    /* Probably stack error - bail out */
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u|",
                          pit.uport,
                          qos_port_conf.qos_remarking_enable,
                          qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START],
                          qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 1],
                          qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 2],
                          qos_port_conf.dscp_remarking_val[VTSS_QUEUE_START + 3]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_BUILD_SMB */
//
// Qos rate limit and shaper
//
static cyg_int32 handler_config_ratelimit(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t     pit;
    qos_port_conf_t conf, newconf;
    int             ct;
    char            var_limiter[32], var_shaper[32];
    int             limiter_rate, limiter_enabled, limiter_unit;
    int             shaper_rate, shaper_enabled, shaper_unit;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                errors++;   /* Probably stack error */
                continue;
            }
            newconf = conf;

            //policer
            /*pe_%d: CHECKBOX */
            sprintf(var_limiter, "pe_%d", pit.uport);
            if (cyg_httpd_form_varable_find(p, var_limiter)) { /* "on" if checked */
                newconf.port_policer[0].enabled = TRUE;
            } else {
                newconf.port_policer[0].enabled = FALSE;
            }
            /* p_%d: TEXT */
            sprintf(var_limiter, "p_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_limiter, &limiter_rate)) {
                newconf.port_policer[0].policer.rate = limiter_rate;
            }
            /* pu_%d: SELECT */
            sprintf(var_limiter, "pu_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_limiter, &limiter_unit)) {
                if (limiter_unit) {
                    newconf.port_policer[0].policer.rate = limiter_rate * 1000;
                }
            }

            //shaper
            /*se_%d: CHECKBOX */
            sprintf(var_shaper, "se_%d", pit.uport);
            if (cyg_httpd_form_varable_find(p, var_shaper)) { /* "on" if checked */
                newconf.shaper_status = RATE_STATUS_ENABLE;
            } else {
                newconf.shaper_status = RATE_STATUS_DISABLE;
            }
            /* s_%d: TEXT */
            sprintf(var_shaper, "s_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_shaper, &shaper_rate)) {
                newconf.shaper_rate = shaper_rate;
            }
            /* su_%d: SELECT */
            sprintf(var_shaper, "su_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_shaper, &shaper_unit)) {
                if (shaper_unit) {
                    newconf.shaper_rate = shaper_rate * 1000;
                }
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                vtss_rc rc;
                if ((rc = qos_port_conf_set(sid, pit.iport, &newconf)) < 0) {
                    T_D("qos_port_conf_set(%u,%d): failed rc = %d", sid, pit.uport, rc);
                    errors++; /* Probably stack error */
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/ratelimit.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &conf) < 0) {
                break;    /* Probably stack error - bail out */
            }
            // policer
            limiter_unit = 0;
            limiter_rate =  conf.port_policer[0].policer.rate;
            limiter_enabled = conf.port_policer[0].enabled;
            if (limiter_rate >= 10000) {
                limiter_rate =  limiter_rate / 1000;
                limiter_unit = 1;
            }

            //shaper
            shaper_unit = 0;
            shaper_rate =  conf.shaper_rate;
            shaper_enabled = conf.shaper_status;
            if (conf.shaper_rate >= 10000) {
                shaper_rate =  conf.shaper_rate / 1000;
                shaper_unit = 1;
            }
            // format: [uport]/[policer enabled]/[policer rate]/[policer unit]|[shaper enabled]/[shaper rate]/[shaper unit]|....
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u|",
                          pit.uport,
                          limiter_enabled,
                          limiter_rate,
                          limiter_unit,
                          shaper_enabled,
                          shaper_rate,
                          shaper_unit);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

/*lint -sem(handler_config_qcl, thread_protected) ... There is only one httpd thread */
static cyg_int32 handler_config_qcl(CYG_HTTPD_STATE *p)
{
    int ct;
    vtss_rc rc;
    vtss_qcl_id_t qcl_id = QCL_ID_START;
    vtss_qce_id_t qce_id = QCE_ID_NONE;
    qos_qce_entry_conf_t qce_conf, new_conf;
    int qce_flag = 0, i, var_value1, var_value2 = 0, var_value3 = 0;
    char var_name1[32];
    static BOOL qce_duplicate = 0;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        memset(&qce_conf, 0x0, sizeof(qce_conf));
        memset(&new_conf, 0x0, sizeof(new_conf));

        /* store form data */
        if (cyg_httpd_form_varable_int(p, "qcl_id", &var_value1)) {
            qcl_id = var_value1;

            if (cyg_httpd_form_varable_int(p, "qce_id", &var_value1)) {
                new_conf.id = var_value1;
            }

            if (cyg_httpd_form_varable_int(p, "next_qce_id", &var_value1)) {
                qce_id = var_value1;
            }

            if (cyg_httpd_form_varable_int(p, "qce_type", &var_value1)) {

                new_conf.type = var_value1;
                switch (new_conf.type) {
                case VTSS_QCE_TYPE_ETYPE:
                    if (cyg_httpd_form_varable_hex(p, "ether_type", (ulong *)&var_value1)) {
                        new_conf.frame.etype.val = var_value1;
                    }
                    if (cyg_httpd_form_varable_int(p, "class", &var_value1)) {
                        new_conf.frame.etype.prio = uprio2iprio(var_value1);
                    }
                    break;

                case VTSS_QCE_TYPE_VLAN:
                    if (cyg_httpd_form_varable_int(p, "vid", &var_value1)) {
                        new_conf.frame.vlan.vid = var_value1;
                    }
                    if (cyg_httpd_form_varable_int(p, "class", &var_value1)) {
                        new_conf.frame.vlan.prio = uprio2iprio(var_value1);
                    }
                    break;

                case VTSS_QCE_TYPE_UDP_TCP:
                    if (cyg_httpd_form_varable_int(p, "port_low", &var_value1)) {
                        new_conf.frame.udp_tcp.low = var_value1;
                    }
                    if (cyg_httpd_form_varable_int(p, "port_high", &var_value1)) {
                        new_conf.frame.udp_tcp.high = var_value1;
                    } else {
                        new_conf.frame.udp_tcp.high = new_conf.frame.udp_tcp.low;
                    }
                    if (cyg_httpd_form_varable_int(p, "class", &var_value1)) {
                        new_conf.frame.udp_tcp.prio  = uprio2iprio(var_value1);
                    }
                    break;

                case VTSS_QCE_TYPE_DSCP:
                    if (cyg_httpd_form_varable_int(p, "dscp", &var_value1)) {
                        new_conf.frame.dscp.dscp_val = var_value1;
                    }
                    if (cyg_httpd_form_varable_int(p, "class", &var_value1)) {
                        new_conf.frame.dscp.prio = uprio2iprio(var_value1);
                    }
                    break;

                case VTSS_QCE_TYPE_TOS:
                    for (i = 0; i <= 7; i++) {
                        sprintf(var_name1, "class%d", i);
                        if (cyg_httpd_form_varable_int(p, var_name1, &var_value1)) {
                            new_conf.frame.tos_prio[i] = uprio2iprio(var_value1);
                        }
                    }
                    break;

                case VTSS_QCE_TYPE_TAG:
                    for (i = 0; i <= 7; i++) {
                        sprintf(var_name1, "class%d", i);
                        if (cyg_httpd_form_varable_int(p, var_name1, &var_value1)) {
                            new_conf.frame.tag_prio[i] = uprio2iprio(var_value1);
                        }
                    }
                    break;

                default:
                    break;
                }

                qce_conf.id = QCE_ID_NONE;
                while (qos_mgmt_qce_entry_get(qcl_id, qce_conf.id, &qce_conf, TRUE) == VTSS_OK) {
                    if (new_conf.id == qce_conf.id) {
                        continue;
                    }
                    if ( (new_conf.type == qce_conf.type) &&
                         ( (new_conf.type == VTSS_QCE_TYPE_ETYPE && new_conf.frame.etype.val == qce_conf.frame.etype.val) ||
                           (new_conf.type == VTSS_QCE_TYPE_VLAN && new_conf.frame.vlan.vid == qce_conf.frame.vlan.vid) ||
                           (new_conf.type == VTSS_QCE_TYPE_UDP_TCP && new_conf.frame.udp_tcp.low == qce_conf.frame.udp_tcp.low && new_conf.frame.udp_tcp.high == qce_conf.frame.udp_tcp.high) ||
                           (new_conf.type == VTSS_QCE_TYPE_DSCP && new_conf.frame.dscp.dscp_val == qce_conf.frame.dscp.dscp_val) ||
                           (new_conf.type == VTSS_QCE_TYPE_TOS && (!memcmp(new_conf.frame.tos_prio, qce_conf.frame.tos_prio, sizeof(new_conf.frame.tos_prio)))) ||
                           (new_conf.type == VTSS_QCE_TYPE_TAG && (!memcmp(new_conf.frame.tos_prio, qce_conf.frame.tos_prio, sizeof(new_conf.frame.tos_prio))))
                         )) {
                        /*
                        char * err = "QCE is repeat in this QCL";
                        send_custom_error(p, err, err, strlen(err));
                        */
                        qce_duplicate = 1;
                        redirect(p, "/qcl.htm");
                        return -1; // Do not further search the file system.
                    }
                }

                //save new configuration
                (void)qos_mgmt_qce_entry_get(qcl_id, new_conf.id, &qce_conf, 0);
                if (memcmp(&new_conf, &qce_conf, sizeof(new_conf)) != 0) {
                    T_D("Calling qos_mgmt_qce_entry_add(%d, %d)", qcl_id, qce_id);
                    if ( ( rc = qos_mgmt_qce_entry_add(qcl_id, qce_id, &new_conf) ) < 0) {
                        T_W("qos_mgmt_qce_entry_add(%d, %d): failed, rc = %x ", qcl_id, qce_id, rc);

                        if ( rc == QOS_ERROR_QCE_TABLE_FULL ) {
                            char *err = "QCE is full";
                            send_custom_error(p, err, err, strlen(err));
                            return -1; // Do not further search the file system.
                        }
                    }
                }
            }
        }

        redirect(p, "/qcl.htm");
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */

        /* Format: [flag]/[date]
            <Del>         : 1/[qcl_id]/[qce_id]
            <Move>        : 2/[qcl_id]/[next_qce_id]
            <Edit>        : 3/[qcl_id]/[qce_id]
            <Insert>      : 4/[qcl_id]/[next_qce_id]
            <Add to Last> : 5/[qcl_id]/0
            <Get>         : 6/[qcl_id]
        */
        if (var_qceConfigFlag[0]) {
            qce_flag = atoi(var_qceConfigFlag);
            if (var_SelectQclId[0]) {
                var_value2 = atoi(var_SelectQclId);
                if (var_SelectQceId[0]) {
                    var_value3 = atoi(var_SelectQceId);
                }
            }
            switch (qce_flag) {
            case 1:
                qcl_id = (vtss_qce_id_t) var_value2;
                (void)qos_mgmt_qce_entry_del(var_value2, var_value3);
                break;
            case 2:
                qcl_id = (vtss_qce_id_t) var_value2;
                if (qos_mgmt_qce_entry_get(var_value2, var_value3, &qce_conf, 1) == VTSS_OK) {
                    (void)qos_mgmt_qce_entry_add(var_value2, var_value3, &qce_conf);
                }
                break;
            case 3:
            case 4:
            case 5:
                qcl_id = (vtss_qce_id_t) var_value2;
                qce_id = (vtss_qce_id_t) var_value3;
                break;
            case 6:
                qcl_id = (vtss_qce_id_t) var_value2;
                break;
            default:
                break;
            }
        }

        cyg_httpd_start_chunked("html");

        if (qce_flag == 4 || qce_flag == 5) { //add-insert
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/0/%u/0/FFFF/1", qcl_id, qce_id);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            cyg_httpd_end_chunked();
            return -1; // Do not further search the file system.
        }

        /* get form data
           Format: [Selected qcl_id],[qce duplicate];[qce_id]/[next_qce_id]/[qce_type]/[type_value_filed]|...,
           [type_value_filed]
           - Ethernet Type [eth_type]/[class]
           - VLAN ID       [vid]/[class]
           - UDP/TCP Port  [port_low]/[port_high]/[class]
           - DSCP          [dscp]/[class]
           - ToS           [class0]/[class1]/[class2]/[class3]/[class4]/[class5]/[class6]/[class7]
           - Tag Priority  [class0]/[class1]/[class2]/[class3]/[class4]/[class5]/[class6]/[class7]
        */
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u;", qcl_id, qce_duplicate);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        qce_duplicate = 0; //reset
        while (qos_mgmt_qce_entry_get(qcl_id, qce_id, &qce_conf, qce_flag == 3 ? 0 : 1) == VTSS_OK) {
            rc = qos_mgmt_qce_entry_get(qcl_id, qce_conf.id, &new_conf, 1) ;

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u",
                          qcl_id,
                          qce_conf.id,
                          (rc == VTSS_OK) ? new_conf.id : QCE_ID_NONE,
                          qce_conf.type);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            if (qce_conf.type == VTSS_QCE_TYPE_ETYPE) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%x/%u", qce_conf.frame.etype.val, iprio2uprio(qce_conf.frame.etype.prio));
            } else if (qce_conf.type == VTSS_QCE_TYPE_VLAN) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u", qce_conf.frame.vlan.vid, iprio2uprio(qce_conf.frame.vlan.prio));
            } else if (qce_conf.type == VTSS_QCE_TYPE_UDP_TCP) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u", qce_conf.frame.udp_tcp.low, qce_conf.frame.udp_tcp.high, iprio2uprio(qce_conf.frame.udp_tcp.prio));
            } else if (qce_conf.type == VTSS_QCE_TYPE_DSCP) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u", qce_conf.frame.dscp.dscp_val, iprio2uprio(qce_conf.frame.dscp.prio));
            } else if (qce_conf.type == VTSS_QCE_TYPE_TOS) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u/%u/%u/%u/%u",
                              iprio2uprio(qce_conf.frame.tos_prio[0]),
                              iprio2uprio(qce_conf.frame.tos_prio[1]),
                              iprio2uprio(qce_conf.frame.tos_prio[2]),
                              iprio2uprio(qce_conf.frame.tos_prio[3]),
                              iprio2uprio(qce_conf.frame.tos_prio[4]),
                              iprio2uprio(qce_conf.frame.tos_prio[5]),
                              iprio2uprio(qce_conf.frame.tos_prio[6]),
                              iprio2uprio(qce_conf.frame.tos_prio[7]));
            } else if (qce_conf.type == VTSS_QCE_TYPE_TAG) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u/%u/%u/%u/%u",
                              iprio2uprio(qce_conf.frame.tag_prio[0]),
                              iprio2uprio(qce_conf.frame.tag_prio[1]),
                              iprio2uprio(qce_conf.frame.tag_prio[2]),
                              iprio2uprio(qce_conf.frame.tag_prio[3]),
                              iprio2uprio(qce_conf.frame.tag_prio[4]),
                              iprio2uprio(qce_conf.frame.tag_prio[5]),
                              iprio2uprio(qce_conf.frame.tag_prio[6]),
                              iprio2uprio(qce_conf.frame.tag_prio[7]));
            }
            cyg_httpd_write_chunked(p->outbuffer, ct);

            if (rc == VTSS_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            qce_id = qce_conf.id;

            if (qce_flag == 3) { //edit
                break;
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qcl_wizard(CYG_HTTPD_STATE *p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_GET) {
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qcl_wizard_policy(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int             ct;
    port_iter_t     pit;
    vtss_qcl_id_t   qcl_no;
    qos_port_conf_t port_conf, port_newconf;
    char            var_name[32], var_name1[32];
    size_t          len;
    const char      *policy_name;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        /* store form data */
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (qos_port_conf_get(sid, pit.iport, &port_conf) != VTSS_OK) {
                errors++;
                continue;
            }
            port_newconf = port_conf;

            sprintf(var_name, "policy_port_%d", pit.uport);
            if ((policy_name = cyg_httpd_form_varable_string(p, var_name, &len)) && len > 0) {
                for (qcl_no = QCL_ID_START; qcl_no <= QCL_ID_END; qcl_no++) {
                    sprintf(var_name1, "policy_%d_port_%d", qcl_no, pit.uport);
                    if (memcmp(policy_name, var_name1, strlen(var_name1)) == 0) {
                        port_newconf.qcl_no = qcl_no;
                        break;
                    }
                }
            }

            if (memcmp(&port_newconf, &port_conf, sizeof(port_newconf)) != 0) {
                T_D("Calling qos_port_conf_set(%d,%d)", sid, pit.uport);
                if (qos_port_conf_set(sid, pit.iport, &port_newconf) < 0) {
                    errors++;
                    T_E("qos_port_conf_set(%d,%d): failed", sid, pit.uport);
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qcl_wizard_finish.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        /* get form data
        Format: [policy_id]/[is_policy_member]/[is_policy_member]/...[is_policy_member]|...
         */
        for (qcl_no = QCL_ID_START; qcl_no <= QCL_ID_END; qcl_no++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u", qcl_no);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (qos_port_conf_get(sid, pit.iport, &port_conf) == VTSS_OK) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",
                                  port_conf.qcl_no == qcl_no ? "1" : "0");
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
            cyg_httpd_write_chunked("|", 1);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qcl_wizard_qce(CYG_HTTPD_STATE *p)
{
    vtss_rc              rc;
    vtss_qcl_id_t        qcl_id = 0;
    vtss_qce_id_t        qce_id = 0;
    qos_qce_entry_conf_t newconf;
    ulong                var_value = 0, var_value1 = 0;
    int                  i, var_value2;
    vtss_prio_t          prio = 0;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        if (cyg_httpd_form_varable_long_int(p, "appl_type", &var_value)) {
            //qcl_id
            if (cyg_httpd_form_varable_int(p, "qcl_id", &var_value2)) {
                qcl_id = var_value2;
            }

            //traffic_class
            var_value2 = 0;
            if (cyg_httpd_form_varable_int(p, "traffic_class", &var_value2)) {
                prio = (uint)var_value2;
            }

            for (i = 0; i < 32; i++) {
                if ((var_value & (1 << i)) == 0) {
                    continue;
                }

                memset(&newconf, 0x0, sizeof(newconf));
                newconf.type = VTSS_QCE_TYPE_UDP_TCP;
                newconf.frame.udp_tcp.prio = prio;

                switch (i) {
                case 0: /* QuickTime 4 Server - TCP 6970 */
                    newconf.frame.udp_tcp.low = 6970;
                    newconf.frame.udp_tcp.high = 6970;
                    break;
                case 1: /* MSN Messenger Phone - TCP 6901 */
                    newconf.frame.udp_tcp.low = 6901;
                    newconf.frame.udp_tcp.high = 6901;
                    break;
                case 2: /* Yahoo Messenger Phone - UDP 5055 */
                    newconf.frame.udp_tcp.low = 5055;
                    newconf.frame.udp_tcp.high = 5055;
                    break;
                case 3: /* Napster - TCP 6699 */
                    newconf.frame.udp_tcp.low = 6699;
                    newconf.frame.udp_tcp.high = 6699;
                    break;
                case 4: /* Real Audio - TCP 7070, UDP 6970-7170 */
                    newconf.frame.udp_tcp.low = 6970;
                    newconf.frame.udp_tcp.high = 7170;
                    break;
                case 5: /* Blizzard Battlenet - TCP 4000, 6112 */
                    newconf.frame.udp_tcp.low = 4000;
                    newconf.frame.udp_tcp.high = 4000;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 6112;
                    newconf.frame.udp_tcp.high = 6112;
                    break;
                case 6: /* Fighter Ace II - TCP 50000-50100
                           for DX play - TCP 47624, 2300-2400 */
                    newconf.frame.udp_tcp.low = 50000;
                    newconf.frame.udp_tcp.high = 50100;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 47624;
                    newconf.frame.udp_tcp.high = 47624;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 2300;
                    newconf.frame.udp_tcp.high = 2400;
                    break;
                case 7: /* Quake2 - UDP 27910 */
                    newconf.frame.udp_tcp.low = 27910;
                    newconf.frame.udp_tcp.high = 27910;
                    break;
                case 8: /* Quake3 - UDP 27660-27662 */
                    newconf.frame.udp_tcp.low = 27660;
                    newconf.frame.udp_tcp.high = 27662;
                    break;
                case 9: /* MSN Game Zone - TCP 6667, 28800-29000
                           for DX play - TCP 47624, 2300-2400 */
                    newconf.frame.udp_tcp.low = 6667;
                    newconf.frame.udp_tcp.high = 6667;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 28800;
                    newconf.frame.udp_tcp.high = 29000;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 47624;
                    newconf.frame.udp_tcp.high = 47624;
                    (void)qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf);
                    qce_id = newconf.id = 0;
                    newconf.frame.udp_tcp.low = 2300;
                    newconf.frame.udp_tcp.high = 2400;
                    break;
                case 10: /* User Define - Ethernet Type */
                    newconf.type = VTSS_QCE_TYPE_ETYPE;
                    newconf.frame.udp_tcp.prio = 0;
                    newconf.frame.etype.prio = prio;
                    if (cyg_httpd_form_varable_hex(p, "user_etype", &var_value1)) {
                        newconf.frame.etype.val = var_value1;
                    }
                    break;
                case 11: /* User Define - VLAN ID */
                    newconf.type = VTSS_QCE_TYPE_VLAN;
                    newconf.frame.udp_tcp.prio = 0;
                    newconf.frame.vlan.prio = prio;
                    if (cyg_httpd_form_varable_int(p, "user_vid", &var_value2)) {
                        newconf.frame.vlan.vid = var_value2;
                    }
                    break;
                case 12: /* User Define - UDP port range */
                    if (cyg_httpd_form_varable_int(p, "user_port_low", &var_value2)) {
                        newconf.frame.udp_tcp.low = var_value2;
                    }
                    if (cyg_httpd_form_varable_int(p, "user_port_high", &var_value2)) {
                        newconf.frame.udp_tcp.high = var_value2;
                    }
                    break;
                case 13: /* User Define - DSCP */
                    newconf.type = VTSS_QCE_TYPE_DSCP;
                    newconf.frame.udp_tcp.prio = 0;
                    newconf.frame.dscp.prio = prio;
                    if (cyg_httpd_form_varable_int(p, "user_dscp", &var_value2)) {
                        newconf.frame.dscp.dscp_val = var_value2;
                    }
                    break;
                default:
                    continue;
                }

                //save new configuration
                T_D("Calling qos_mgmt_qce_entry_add(%d, %d)", qcl_id, qce_id);
                if ( ( rc = qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf) ) < 0) {
                    if ( rc == QOS_ERROR_QCE_TABLE_FULL ) {
                        char *err = "QCE is full";
                        send_custom_error(p, err, err, strlen(err));
                        return -1; // Do not further search the file system.
                    } else if (rc != VTSS_OK) {
                        T_E("qos_mgmt_qce_entry_add(%d, %d): failed", qcl_id, qce_id);
                    }
                }
            }
        }
        redirect(p, "/qcl_wizard_finish.htm");
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qcl_wizard_tos(CYG_HTTPD_STATE *p)
{
    vtss_rc              rc;
    vtss_qcl_id_t        qcl_id = 0;
    vtss_qce_id_t        qce_id = 0;
    qos_qce_entry_conf_t newconf;
    int                  i, var_value2;
    char                 var_name1[32];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        memset(&newconf, 0x0, sizeof(newconf));
        newconf.type = VTSS_QCE_TYPE_TOS;

        //qcl_id
        if (cyg_httpd_form_varable_int(p, "qcl_id", &var_value2)) {
            qcl_id = var_value2;
        }

        //traffic_class
        for (i = 0; i <= 7; i++) {
            sprintf(var_name1, "traffic_class%d", i);
            if (cyg_httpd_form_varable_int(p, var_name1, &var_value2)) {
                newconf.frame.tos_prio[i] = (uint)var_value2;
            }
        }

        //save new configuration
        T_D("Calling qos_mgmt_qce_entry_add(%d, %d)", qcl_id, qce_id);
        if ((rc = qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf)) < 0) {
            if (rc == QOS_ERROR_QCE_TABLE_FULL) {
                char *err = "QCE is full";
                send_custom_error(p, err, err, strlen(err));
                return -1; // Do not further search the file system.
            } else if (rc != VTSS_OK) {
                T_E("qos_mgmt_qce_entry_add(%d, %d): failed", qcl_id, qce_id);
            }
        }
        redirect(p, "/qcl_wizard_finish.htm");
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_config_qcl_wizard_tag(CYG_HTTPD_STATE *p)
{
    vtss_rc              rc;
    vtss_qcl_id_t        qcl_id = 0;
    vtss_qce_id_t        qce_id = 0;
    qos_qce_entry_conf_t newconf;
    int                  i, var_value2;
    char                 var_name1[32];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        memset(&newconf, 0x0, sizeof(newconf));
        newconf.type = VTSS_QCE_TYPE_TAG;

        //qcl_id
        if (cyg_httpd_form_varable_int(p, "qcl_id", &var_value2)) {
            qcl_id = var_value2;
        }

        //traffic_class
        for (i = 0; i <= 7; i++) {
            sprintf(var_name1, "traffic_class%d", i);
            if (cyg_httpd_form_varable_int(p, var_name1, &var_value2)) {
                newconf.frame.tag_prio[i] = (uint)var_value2;
            }
        }

        //save new configuration
        T_D("Calling qos_mgmt_qce_entry_add(%d, %d)", qcl_id, qce_id);
        if ((rc = qos_mgmt_qce_entry_add(qcl_id, qce_id, &newconf)) < 0) {
            if (rc == QOS_ERROR_QCE_TABLE_FULL) {
                char *err = "QCE is full";
                send_custom_error(p, err, err, strlen(err));
                return -1; // Do not further search the file system.
            } else if (rc != VTSS_OK) {
                T_E("qos_mgmt_qce_entry_add(%d, %d): failed", qcl_id, qce_id);
            }
        }
        redirect(p, "/qcl_wizard_finish.htm");
    }

    return -1; // Do not further search the file system.
}
#else
#error "Unsupported architecture"
#endif

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t qos_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    int i;
    unsigned int cnt = 0;
    char dscp_names[64 * 10];
    char buff[QOS_WEB_BUF_LEN + sizeof(dscp_names)];

    for (i = 0; i < 64; i++) {
        cnt += snprintf(dscp_names + cnt, sizeof(dscp_names) - cnt, "%s'%s'", i == 0 ? "" : ",", qos_dscp2str(i));
    }

    (void) snprintf(buff, sizeof(buff),
                    "var configQosClassMax = %d;\n"
                    "var configQosBitRateMin = %d;\n"
                    "var configQosBitRateMax = %d;\n"
                    "var configQosBitRateDef = %d;\n"
                    "var configQosDplMax = %d;\n"
                    "var configQCLMax = %d;\n"
                    "var configQCEMax = %d;\n"
                    "var configQosDscpNames = [%s];\n",
                    QOS_CLASS_CNT,
                    QOS_BITRATE_MIN,
                    QOS_BITRATE_MAX,
                    QOS_BITRATE_DEF,
                    QOS_DPL_MAX,
                    QCL_MAX,
                    QCE_MAX,
                    dscp_names);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(qos_lib_config_js);

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t qos_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[QOS_WEB_BUF_LEN];
    (void) snprintf(buff, sizeof(buff), "%s" // <---- Add at least one string if the defines below ends up in nothing

#if defined(QOS_USE_FIXED_PCP_QOS_MAP)
                    ".has_qos_pcp_dei { display: none; }\r\n"
                    ".has_qos_tag_class { display: none; }\r\n"
#else
                    ".has_qos_fixed_map { display: none; }\r\n"
#endif

#if !defined(VTSS_SW_OPTION_BUILD_SMB) && !defined(VTSS_SW_OPTION_BUILD_CE)
                    ".has_qos_tag_class { display: none; }\r\n"
                    ".has_qos_dscp_class { display: none; }\r\n"
#endif
                    , ""); // <---- This is the (empty) string that is always added
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(qos_lib_filter_css);
#endif /* defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL) */

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

/****************************************************************************/
/*  Common Luton28/JAGUAR/Luton26/Serval table entries                      */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_qos_counter, "/stat/qos_counter",   handler_stat_qos_counter);

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_stormctrl, "/config/stormconfig", handler_config_stormctrl);
#endif

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
/****************************************************************************/
/*  JAGUAR/Luton26/Serval specific table entries                            */
/****************************************************************************/
#ifdef VTSS_FEATURE_QCL_V2
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qcl_v2, "/config/qcl_v2", handler_config_qcl_v2);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_qcl_v2,   "/stat/qcl_v2",   handler_stat_qcl_v2);
#endif
#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_classification,      "/config/qos_port_classification",      handler_config_qos_port_classification);
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_classification_map,  "/config/qos_port_classification_map",  handler_config_qos_port_classification_map);
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_dscp_classification_map,  "/config/qos_dscp_classification_map",  handler_config_qos_dscp_classification_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_dscp_translation, "/config/qos_dscp_translation", handler_config_dscp_translation);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dscp_port_config, "/config/dscp_port_config", handler_config_dscp_port_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dscp_bsd_qos_ingr_cls, "/config/dscp_based_qos_ingr_cls", handler_config_dscp_based_qos_ingr_classi);
#endif /* (VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL) && defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_policers_multi,      "/config/qos_port_policers_multi",      handler_config_qos_port_policers_multi);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_policer_edit_multi,  "/config/qos_port_policer_edit_multi",  handler_config_qos_port_policer_edit_multi);
#else
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_policers,            "/config/qos_port_policers",            handler_config_qos_port_policers);
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL) && defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM) */
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS) */
#if defined(VTSS_SW_OPTION_BUILD_CE)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_queue_policers,           "/config/qos_queue_policers",           handler_config_qos_queue_policers);
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_schedulers,          "/config/qos_port_schedulers",          handler_config_qos_port_schedulers);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_shapers,             "/config/qos_port_shapers",             handler_config_qos_port_shapers);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_scheduler_edit,      "/config/qos_port_scheduler_edit",      handler_config_qos_port_scheduler_edit);
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_tag_remarking,       "/config/qos_port_tag_remarking",       handler_config_qos_port_tag_remarking);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_tag_remarking_edit,  "/config/qos_port_tag_remarking_edit",  handler_config_qos_port_tag_remarking_edit);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_stormctrl_jr,                 "/config/stormctrl_jr",                 handler_config_stormctrl_jr);
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#ifdef VTSS_FEATURE_QOS_WRED
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_wred, "/config/qos_wred", handler_config_qos_wred);
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_VSTAX_V2)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_cmef, "/config/qos_cmef", handler_config_qos_cmef);
#endif /* VTSS_FEATURE_VSTAX_V2 */

#elif defined(VTSS_ARCH_LUTON28)
/****************************************************************************/
/*  LUTON28 specific table entries                                          */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_class,       "/config/classconfig",       handler_config_qos_class);
#ifdef VTSS_SW_OPTION_BUILD_SMB
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dscp_remarking,  "/config/dscp_remarking",    handler_config_dscp_remarking);
#endif /* VTSS_SW_OPTION_BUILD_SMB */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ratelimit,       "/config/ratelimit",         handler_config_ratelimit);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qcl,             "/config/qcl",               handler_config_qcl);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qclwizard,       "/config/qcl_wizard",        handler_config_qcl_wizard);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qclwizardpolicy, "/config/qcl_wizard_policy", handler_config_qcl_wizard_policy);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qclwizardqce,    "/config/qcl_wizard_qce",    handler_config_qcl_wizard_qce);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qclwizardtos,    "/config/qcl_wizard_tos",    handler_config_qcl_wizard_tos);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qclwizardtag,    "/config/qcl_wizard_tag",    handler_config_qcl_wizard_tag);
#else
#error "Unsupported architecture"
#endif
