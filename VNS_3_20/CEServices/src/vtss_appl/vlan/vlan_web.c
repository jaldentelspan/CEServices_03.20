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
#include "vlan_api.h"
#include "mgmt_api.h"
#include "port_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* Temporary disabled while XXRP is under construction */
#undef VTSS_SW_OPTION_MVRP

#define VLAN_WEB_BUF_LEN 512

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

#define VTSS_FORBID_BF_SIZE(n)      (((n)+3)/4)
#define VTSS_PORT_FORBID_BF_SET(a, n, v) { if (v) { a[(n)/4] |= (v<<((n%4)*2)); }}
#define VTSS_PORT_FORBID_BF_SIZE   VTSS_FORBID_BF_SIZE(VTSS_PORTS)


/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static cyg_int32 handler_config_vlan_ports(CYG_HTTPD_STATE* p)
{
    vtss_isid_t           sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vlan_port_conf_t      conf;
    int                   ct;
    port_iter_t           pit;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    u16                   tpid;
#endif

    //    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
    //        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN))
        return -1;
#endif

    if(p->method == CYG_HTTPD_METHOD_POST) {
        int              errors = 0;
        vlan_port_conf_t newconf;
        char             var_ingressflt[32], var_frametype[32], var_pvid[32];
        char             var_porttype[32];
        char             var_tx_tag[32];
        int              port_type;
        int              pvid, frame_type, tx_tag;

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        if (cyg_httpd_form_varable_hex(p, "tpid", (ulong *)&tpid)) {
            if (vlan_mgmt_vlan_tpid_set(tpid) != VTSS_RC_OK) {
                T_D("Unable to set TPID\n");
            }
        }
#endif
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if(vlan_mgmt_port_conf_get(sid, pit.iport, &conf, VLAN_USER_STATIC) < 0) {
                errors++; /* Probably stack error */
                continue;
            }
            newconf = conf;

            /* ingressflt_%d: CHECKBOX */
            sprintf(var_ingressflt, "ingressflt_%d", pit.uport);
            newconf.ingress_filter = FALSE;
            if(cyg_httpd_form_varable_find(p, var_ingressflt)) /* "on" if checked */
                newconf.ingress_filter = TRUE;

            /* tx_tag_%d: CHECKBOX */
            sprintf(var_tx_tag, "tx_tag_%d", pit.uport);
            if(cyg_httpd_form_varable_int(p, var_tx_tag, &tx_tag)) {
                newconf.tx_tag_type = tx_tag;
            }

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
            /* frametypev2_%d: SELECTCELL */
            sprintf(var_frametype, "frametypev2_%d", pit.uport);
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
            /* frametypev1_%d: SELECTCELL */
            sprintf(var_frametype, "frametypev1_%d", pit.uport);
#endif
            /* "untagged" if 2, "tagged" if 1, All if 0 */
            if(cyg_httpd_form_varable_int(p, var_frametype, &frame_type)) {
                newconf.frame_type = frame_type;
            }
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
            /* porttypev2_%d: SELECTCELL */
            sprintf(var_porttype, "porttypev2_%d", pit.uport);
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
            /* porttypev1_%d: SELECTCELL */
            sprintf(var_porttype, "porttypev1_%d", pit.uport);
#endif
            /* "unaware" if 0, C-port if 1, S-port if 2, S-custom-port */
            if(cyg_httpd_form_varable_int(p, var_porttype, &port_type)) {
                newconf.port_type = port_type;
            }

            /* selpvlan_%d: bool */
            sprintf(var_pvid, "selpvlan_%d", pit.uport);
            if(cyg_httpd_form_varable_int(p, var_pvid, &pvid) && pvid == 0 /* Specific */) {
                /* pvid_%d: TEXT */
                sprintf(var_pvid, "pvid_%d", pit.uport);
                if(cyg_httpd_form_varable_int(p, var_pvid, &pvid) &&
                        pvid >= VTSS_VID_NULL &&
                        pvid < VTSS_VIDS) {
                    newconf.pvid = (uint)pvid; /* Specific Port VLAN ID */
                }
            } else
                newconf.pvid = VTSS_VID_NULL; /* None */
            newconf.untagged_vid =  newconf.pvid;

            if(memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                if(vlan_mgmt_port_conf_set(sid, pit.iport, &newconf, VLAN_USER_STATIC) < 0) {
                    T_E("vlan_mgmt_port_conf_set(%d): failed", pit.iport);
                    errors++; /* Probably stack error */
                }
#ifdef VTSS_SW_OPTION_ACL
                mgmt_vlan_pvid_change(sid, pit.iport, newconf.pvid, conf.pvid);
#endif /* VTSS_SW_OPTION_ACL */
            }
        }
        redirect(p, "/vlan_port.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        if (vlan_mgmt_vlan_tpid_get((vtss_etype_t *)&tpid) != VTSS_RC_OK) {
            T_D("Unable to get TPID\n");
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%x#", tpid);
        cyg_httpd_write_chunked(p->outbuffer, ct);
#else
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%x#", 0x8100);
        cyg_httpd_write_chunked(p->outbuffer, ct);
#endif
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if(vlan_mgmt_port_conf_get(sid, pit.iport, &conf, VLAN_USER_STATIC) < 0) {
                T_E("vlan_port GET error, sid %u, iport %u", sid, pit.iport);
                continue; /* Probably stack error */
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u|",
                    pit.uport,
                    conf.port_type,
                    conf.ingress_filter,
                    conf.frame_type,
                    conf.pvid,
                    conf.tx_tag_type,
                    conf.untagged_vid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

cyg_int32 handler_vlan_membership_stat(CYG_HTTPD_STATE* p)
{
    vtss_isid_t         sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vlan_user_t         VLANuser = VLAN_USER_ALL;
    int                 ct;
    vtss_rc             rc1= VTSS_RC_ERROR;
    vtss_rc             rc2= VTSS_RC_ERROR;
    vlan_mgmt_entry_t   mgmt_conf;
    vlan_mgmt_entry_t   fb_mgmt_conf;
    vid_t               prev_vid = 0;
    static int          prev_number_of_entries = 20;
    static int          prev_start_vid = 1;
    int                 start_vid = -1;
    int                 number_of_entries = -1,  number_of_entries_left;
    char                *user;
    port_iter_t         pit;
    BOOL                mport, fport;

    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN))
        return -1;
#endif/* CYG_HTTPD_METHOD_GET (+HEAD) */
    
    number_of_entries = atoi(var_dyn_num_of_ent);
    if(number_of_entries < 1) {
        number_of_entries = prev_number_of_entries;
    }
    
    start_vid = atoi(var_dyn_start_vid);
    if (start_vid < VLAN_TAG_MIN_VALID || start_vid > VLAN_TAG_MAX_VALID) {
        start_vid = prev_start_vid;
    }

    cyg_httpd_start_chunked("html");

    for (VLANuser = VLAN_USER_STATIC; VLANuser <= VLAN_USER_ALL; VLANuser++) {
        number_of_entries_left = number_of_entries;
        prev_vid = start_vid;
        user = vlan_mgmt_vlan_user_to_txt(VLANuser);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s",user);

        cyg_httpd_write_chunked(p->outbuffer, ct);
        if(VLANuser == VLAN_USER_STATIC || VLANuser == VLAN_USER_ALL) {
            /* get the vid from database and perform respection operation on that vid*/
            while((number_of_entries_left > 0)  &&  (prev_vid <= VLAN_MAX)) {
                mgmt_conf.vid = 0;
                fb_mgmt_conf.vid = 0;
                memset(mgmt_conf.ports, 0, sizeof(mgmt_conf.ports));
                memset(fb_mgmt_conf.ports, 0, sizeof(fb_mgmt_conf.ports));
                rc1 = vlan_mgmt_vlan_get(sid, prev_vid, &mgmt_conf, 0, VLANuser, VLAN_MEMBERSHIP_OP);
                rc2 = vlan_mgmt_vlan_get(sid, prev_vid, &fb_mgmt_conf, 0, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP);
                    
                if ((rc1 == VTSS_OK) || (rc2 == VTSS_OK)) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u",(mgmt_conf.vid)|(fb_mgmt_conf.vid));
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        mport = (mgmt_conf.ports[pit.iport] == VLAN_MEMBER_PORT) ? 1 : 0;
                        fport = (fb_mgmt_conf.ports[pit.iport] == VLAN_FORBIDDEN_PORT) ? 1 : 0;
                        // 3 represents conflict port, 2 represents the forbidden port and 1 represents the normal port
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", (mport | (fport * 2)));
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                    number_of_entries_left--;
                }
                prev_vid++;
                /*End of static user vids list*/
            }
        } else { /* Forbidden ports cannot be configured by volatile users */
            while ((number_of_entries_left > 0) && (vlan_mgmt_vlan_get(sid, prev_vid, &mgmt_conf, 1, VLANuser, VLAN_MEMBERSHIP_OP) 
                   == VTSS_OK)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u", mgmt_conf.vid);
                cyg_httpd_write_chunked(p->outbuffer, ct);
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    mport = (mgmt_conf.ports[pit.iport] == VLAN_MEMBER_PORT) ? 1 : 0;
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", mport);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                prev_vid = mgmt_conf.vid;
                number_of_entries_left--;
            }
        }
    }
    
    prev_number_of_entries = number_of_entries;
    prev_start_vid = start_vid;
    
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u", start_vid);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u", number_of_entries);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    cyg_httpd_end_chunked();
    return -1;
}

cyg_int32 handler_vlan_ports_stat(CYG_HTTPD_STATE* p)
{
    vtss_isid_t             sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vlan_port_conf_t        tconf;
    static vlan_user_t      VLANuser=VLAN_USER_STATIC;
    int                     ct;
    vlan_port_conflicts_t   conflicts;
    char                    *user;
    int                     i;
    u8                      bit_mask, all_usrs = 0 ;
    vlan_port_flags_idx_t   temp;
    BOOL                    conflict_exists;
    port_iter_t             pit;

    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN))
        return -1;
#endif /* CYG_HTTPD_METHOD_GET (+HEAD) */
    
    cyg_httpd_start_chunked("html");
    for(VLANuser = VLAN_USER_STATIC; VLANuser <= VLAN_USER_ALL; VLANuser++) {
        user = vlan_mgmt_vlan_user_to_txt(VLANuser);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s",user);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            memset(&conflicts, 0, sizeof(conflicts));
            if ((vlan_mgmt_port_conf_get(sid, pit.iport, &tconf,VLANuser) < 0) 
                ||(vlan_mgmt_conflicts_get(&conflicts, sid, pit.iport) < 0)) {
                T_E("vlan_port GET error, sid %u, iport %u", sid, pit.iport);
                continue; /* Probably stack error */
            }
            for (temp = VLAN_PORT_PVID_IDX; temp <= VLAN_PORT_TX_TAG_TYPE_IDX; temp++) {
                all_usrs |= conflicts.vlan_users[temp];
            }		
            if (conflicts.port_flags == 0) {
                conflict_exists = FALSE; 
            } else {
                if (all_usrs & (1 << (u8)VLANuser)) {
                    conflict_exists = TRUE; 
                } else {
                    conflict_exists = FALSE;  
                }
            }
            if (tconf.flags) {
                if (tconf.flags != 0x1F) {
                    vlan_port_conf_t conf = {0xffff, 0xffff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};
                    for (i = VLAN_PORT_FLAGS_PVID; i <= VLAN_PORT_FLAGS_TX_TAG_TYPE; i++) {

                        bit_mask = tconf.flags & i;

                        if (bit_mask ==  VLAN_PORT_FLAGS_PVID) {
                            conf.pvid = tconf.pvid;
                        }

                        if (bit_mask ==  VLAN_PORT_FLAGS_AWARE) {
                            conf.port_type = tconf.port_type;
                        }

                        if (bit_mask ==  VLAN_PORT_FLAGS_INGR_FILT) {
                            conf.ingress_filter = tconf.ingress_filter;
                        }

                        if (bit_mask ==  VLAN_PORT_FLAGS_RX_TAG_TYPE) {
                            conf.frame_type = tconf.frame_type;
                        }

                        if (bit_mask ==  VLAN_PORT_FLAGS_TX_TAG_TYPE) {
                            conf.untagged_vid = tconf.untagged_vid;
                            conf.tx_tag_type = tconf.tx_tag_type;
                        }
                    } /*for (i = )*/
                    tconf = conf;
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u/%u/%u/%u/%u/%u/%u/%s",
                        (unsigned int)pit.uport,
                        tconf.pvid,
                        tconf.port_type,
                        tconf.ingress_filter,
                        tconf.frame_type,
                        tconf.tx_tag_type,
                        tconf.untagged_vid,
                        conflict_exists ? "yes" : "No");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
    }
    cyg_httpd_end_chunked();
    return -1;
}

static void send_all_vlans_conf(CYG_HTTPD_STATE* p)
{
    vtss_isid_t                 sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vlan_mgmt_entry_t           conf;
    vtss_vid_t                  vid;
    int                         ct;
    u8                          vlan_flags[(VLAN_TAG_MAX_VALID + 1)/8];
    BOOL                        first = TRUE;

    vid = VTSS_VID_NULL;
    memset(vlan_flags, 0, sizeof(vlan_flags));
    while (vlan_mgmt_vlan_get(sid, vid, &conf, 1, VLAN_USER_STATIC, VLAN_MIXED_OP) == VTSS_OK) {
        if (first) {
            first = FALSE;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "$%u,", conf.vid);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", conf.vid);
        }
        cyg_httpd_write_chunked(p->outbuffer, ct);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s$", conf.vlan_name);
#else
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s$", "");
#endif
        cyg_httpd_write_chunked(p->outbuffer, ct);
        VTSS_BF_SET(vlan_flags, conf.vid, 1);
        vid = conf.vid;
    }
}
static cyg_int32 handler_config_vlan(CYG_HTTPD_STATE* p)
{
    vtss_isid_t                 sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vlan_mgmt_entry_t           conf;
    vtss_vid_t                  vid_list;
    BOOL                        vid_just_added = FALSE;
    BOOL                        vid_exist = FALSE, name_exists = FALSE;
    vid_t                       this_vid_is_new[VLAN_MAX];
    static int                  prev_start_vid = 1;
    uchar                       ports_bf[VTSS_PORT_FORBID_BF_SIZE];
    vtss_rc                     rc= VTSS_RC_ERROR;
    int                         start_vid = -1,stack_wide_vid = 0;
    static int                  prev_number_of_entries = 20;
    vlan_registration_type_t    port_registration[VTSS_PORT_ARRAY_SIZE];
    int                         number_of_entries,number_of_entries_left,number_of_entries_deleted;
    port_iter_t                 pit;
    u32                         port_count;

    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN))
        return -1;
#endif
    if(p->method == CYG_HTTPD_METHOD_POST) {
        vlan_mgmt_entry_t newconf;
        char search_str[32];
        int errors = 0;
        int new_vid, new_entry, i;
        int  port_vlaue;
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        const i8  *str;
        size_t    len;
        vlan_mgmt_vlan_name_conf_t  name_conf;
#endif

        // Detele the VLANs
        vid_list = prev_start_vid;

        // We need to know how many entries that is delete when that save button is pressed to
        // make sure that we do not loop through more entries than the number of valid entries shown
        // at the web page.
        number_of_entries_deleted = 0;
        /* get the vid from database and perform respection operation on that vid.
           Multifobid function is to get all vids in forbidden port list and vlan port list*/
        while(vid_list <= VLAN_MAX) {
            stack_wide_vid = vid_list;
            vid_exist = FALSE;
            /*check if vid is in vlan port list*/
            if(vlan_mgmt_vlan_get(sid, stack_wide_vid, &conf, 0, VLAN_USER_STATIC, VLAN_MIXED_OP) == VTSS_OK) {
                vid_exist = TRUE;
            }
            sprintf(search_str, "delete_%d", stack_wide_vid); 
            if(cyg_httpd_form_varable_find(p, search_str)) {
                T_D("Deleting vlan : %d",stack_wide_vid);
                number_of_entries_deleted ++;

                // Do the deletion if vlan is in vlan port list
                if(vid_exist) {
                    if (vlan_mgmt_vlan_del(sid, conf.vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                        T_N("vlan_mgmt_vlan_del(%d): failed", conf.vid);
                    }
                    if (vlan_mgmt_vlan_del(sid, conf.vid, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP) != VTSS_OK) { 
                        T_N("vlan_mgmt_vlan_del - forbidden(%d): failed", conf.vid);
                    }
                } /* if(vid_exist) */
            } /* if(cyg_httpd_form_varable_find(p, search_str)) */
            vid_list++; // go to next entry
        }

        /* Check for Add new VLANs to ADD*/
        for (new_entry = 1; new_entry < VLAN_MAX; new_entry++) {
            this_vid_is_new[new_entry] = 0;

            /* Add new VLAN */
            new_vid = new_entry;
            sprintf(search_str, "vid_new_%d", new_vid);

            if(cyg_httpd_form_varable_int(p, search_str, &new_vid) && new_vid > VTSS_VID_NULL && new_vid < VTSS_VIDS) {
                /* update vid info in newconf.vid as to compare later*/
                newconf.vid = 0xfff & (uint)new_vid;
                this_vid_is_new[new_entry] = newconf.vid;

#ifdef  VTSS_SW_OPTION_VLAN_NAME
                /*update the vlan name if exist*/
                sprintf(search_str, "name_new_%d", new_entry);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (len != 0) {
                        name_conf.vid = new_vid;
                        memcpy(name_conf.vlan_name, str, len);
                        name_conf.vlan_name[len] = '\0'; 
                        if ((vlan_mgmt_vlan_name_add(&name_conf, 1)) != VTSS_RC_OK) {
                            T_D("vlan_mgmt_vlan_name_add failed for vid %d\n", new_vid);
                        }
                    }
                }
#endif
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    newconf.ports[pit.iport] = FALSE;
                    /* check each port for new vlan id if vlan port list is checked then newconf.ports[pit.iport] is true */
                    sprintf(search_str, "hidden_mask_new_%d_%d", new_entry, pit.uport);
                    if(cyg_httpd_form_varable_int(p, search_str, &port_vlaue)) {
                        if(port_vlaue == 1) {
                            /* here 1 indicates the vlan is checked for vlan port list in web page and its value is 1*/
                            newconf.ports[pit.iport] = VLAN_MEMBER_PORT;
                        } else if(port_vlaue == 2) {
                            /* here 2 indicates the vlan is checked for vlan port list in web page and its value is 2*/
                            newconf.ports[pit.iport] = VLAN_FORBIDDEN_PORT;
                        } else {
                            newconf.ports[pit.iport] = VLAN_NON_MEMBER_PORT;
                        }
                    } /* if(cyg_httpd_form_varable_int(p, search_str, &port_vlaue)) */
                } /* while (port_iter_getnext(&pit)) */

                T_D("Add vlan : %d",newconf.vid);
                rc = vlan_mgmt_vlan_add(sid, &newconf, VLAN_USER_STATIC);
                if((rc != VTSS_OK) && (rc != VLAN_ERROR_DEL_INSTEAD_OF_ADD)) {
                    T_E("vlan_mgmt_vlan_add(%d): failed", new_vid);
                } /* if((rc != VTSS_OK) && (rc != VLAN_ERROR_DEL_INSTEAD_OF_ADD)) */
            } /* if(cyg_httpd_form_varable_int(p, search_str, &new_vid) && new_vid > VTSS_VID_NULL && new_vid < VTSS_VIDS) */
        } /* for (new_entry = 1; new_entry < VLAN_MAX; new_entry++) */

        vid_list = prev_start_vid;
        // determine how many entries that are shown at the web page at the moment
        number_of_entries_left = prev_number_of_entries - number_of_entries_deleted; 

        /* get the vid from database and perform respection operation on that vid.
           Multifobid function is to get all vids in forbidden port list and vlan port list*/
        while((number_of_entries_left > 0) && (vid_list <= VLAN_MAX)) {
            // check/update any of the VLAN mask that has changed
            T_D("number_of_entries_left = %d, vid = %d",number_of_entries_left, vid_list);
            stack_wide_vid = vid_list;
            // Notice: Throughout this while(): 'prev_vid' variable holds
            // the value of the previous entry. The current entry is held in
            // conf.vid.
            vid_just_added = FALSE;
            for (i = 1; i < VLAN_MAX; i++) {
                vid_just_added |= (this_vid_is_new[i] == vid_list);
            }
            if(vid_just_added) {
                // Move on to the next vid
                vid_list++;
                continue;
            }
            /* initialized the bool to false indicates that no vid in forbidden and vlan port list*/
            vid_exist = FALSE;
            memset(&newconf, 0, sizeof(newconf));
            conf.vid = stack_wide_vid;
            newconf.vid = stack_wide_vid;
            /* check the vid exists in vlan portlist*/
            if(vlan_mgmt_vlan_get(sid, stack_wide_vid, &conf, 0, VLAN_USER_STATIC, VLAN_MIXED_OP) == VTSS_OK) {
                vid_exist = TRUE;
            }
            /* if vid exist in eigther of table*/
            if(vid_exist) {
                number_of_entries_left--;
#ifdef  VTSS_SW_OPTION_VLAN_NAME
                /* This is required for comparison */
                sprintf(search_str, "name_%d", newconf.vid);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (len != 0) {
                        memcpy(newconf.vlan_name, str, len);
                        newconf.vlan_name[len] = '\0';
                    }
                }
#endif
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    newconf.ports[pit.iport] = FALSE;
                    /* get the port list for given vid from web*/
                    sprintf(search_str, "hidden_mask_%d_%d", conf.vid, pit.uport);
                    if(cyg_httpd_form_varable_int(p, search_str, &port_vlaue)) {
                        /* here 1 indicates the vlan is checked for vlan port list in web page and its value is 1*/
                        if(port_vlaue == 1) {
                            /* if any ports are included in VLAN */
                            newconf.ports[pit.iport] = VLAN_MEMBER_PORT;
                        } else if(port_vlaue == 2) {
                            /* here 2 indicates the vlan is checked for vlan port list in web page and its value is 2
                               if any ports are included in forbidden port list */
                            newconf.ports[pit.iport] = VLAN_FORBIDDEN_PORT;
                        } else {
                            newconf.ports[pit.iport] = VLAN_NON_MEMBER_PORT;
                        }
                    } /* if(cyg_httpd_form_varable_int(p, search_str, &port_vlaue)) */
                } /* while (port_iter_getnext(&pit)) */
                /* check if any change in VLAN included port list if so then update the VLAN port table*/
                if(memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                    T_D("Updating vlan membership : %d",newconf.vid);
                    /* Clean the old configuration so that there will be no conflicts */
                    (void)vlan_mgmt_vlan_del(sid, newconf.vid, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP);
                    (void)vlan_mgmt_vlan_del(sid, newconf.vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP);
                    /* Add both forbidden and member configuration */
                    rc = vlan_mgmt_vlan_add(sid, &newconf, VLAN_USER_STATIC);
                    if(rc != VTSS_OK && rc != VLAN_ERROR_DEL_INSTEAD_OF_ADD ) {
                        T_E("vlan_mgmt_vlan_add(%d) sid %u: failed", newconf.vid, sid);
                    }
                }
#ifdef  VTSS_SW_OPTION_VLAN_NAME
                sprintf(search_str, "name_%d", newconf.vid);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (len != 0) {
                        name_conf.vid = newconf.vid;
                        memcpy(name_conf.vlan_name, str, len);
                        name_conf.vlan_name[len] = '\0'; 
                        if ((vlan_mgmt_vlan_name_add(&name_conf, 1)) != VTSS_RC_OK) {
                            T_D("vlan_mgmt_vlan_name_add failed for vid %d\n", newconf.vid);
                        }
                    } else {
                        if (vlan_mgmt_vlan_name_del_by_vid(newconf.vid) != VTSS_RC_OK) {
                            T_D("No valid VLAN name exists for vid = %d\n", newconf.vid);
                        }
                    } /* else of if (len != 0) */
                } /* if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) */
#endif
            } /* if(vid_exist) */
            // Move on to the next entry.
            vid_list++;
        } /* while((number_of_entries_left > 0) && (vid_list <= VLAN_MAX)) */

        // Get the start vid from the web page.
        prev_start_vid = httpd_form_get_value_int(p, "StartVid", 1, 4095);
        // Get the number of number of entries shown per page. Here we don't care about how many
        // that can de shown in per page. This is controlled in the vlan.htm code. We simply set the
        // max value to a "high" number (9999).
        prev_number_of_entries = httpd_form_get_value_int(p, "NumberOfEntries", 1, 9999);

        redirect(p, errors ? STACK_ERR_URL : "/vlan.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        // When the save button is pressed the number of entries aren't passed via the var_dyn_num_of_ent. The number
        // of entries shall in this case be set to last known value.
        number_of_entries = atoi(var_dyn_num_of_ent);
        if (number_of_entries < 1) {
            number_of_entries = prev_number_of_entries;
        }

        // Get which vid that shall be the first entry shown in the web page.
        // As with the number of entries we shall simply use the last known value for start vid when the save button is pressed.
        start_vid = atoi(var_dyn_start_vid);
        if (start_vid < VLAN_TAG_MIN_VALID || start_vid > VLAN_TAG_MAX_VALID) {
            start_vid = prev_start_vid;
        }
        int ct;
        int pb_index;
        cyg_httpd_start_chunked("html");

        // Pass start vid and number of entries shown per page to the browser.
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|", start_vid);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u", number_of_entries);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        T_D("start_vid = %d, var_dyn_start_vid = %s, number_of_entries =%d",
                start_vid,var_dyn_start_vid,number_of_entries);

        stack_wide_vid = start_vid; // Subtract one because we are going to ask for the NEXT entry.
        number_of_entries_left = number_of_entries; // Used to determine how many entries that shall be passed to browser
        vid_list = stack_wide_vid;
        // Pass vlan configuration to the browser.
        while(number_of_entries_left > 0 && vid_list <= VLAN_MAX) {
            if (vlan_mgmt_vlan_get(sid, vid_list, &conf, 0, VLAN_USER_STATIC, VLAN_MIXED_OP) == VTSS_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u,", conf.vid);
                cyg_httpd_write_chunked(p->outbuffer, ct);

#ifdef VTSS_SW_OPTION_VLAN_NAME
                name_exists = TRUE;
#endif
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", name_exists);
                cyg_httpd_write_chunked(p->outbuffer, ct);
#ifdef VTSS_SW_OPTION_VLAN_NAME
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s,", conf.vlan_name);
                cyg_httpd_write_chunked(p->outbuffer, ct);
#endif
                T_N("number_of_entries_left = %d, vid = %d",number_of_entries_left,conf.vid);

                memset(port_registration, 0, sizeof(port_registration));
                vlan_mgmt_vlan_registration_get(sid, vid_list, port_registration, VLAN_USER_STATIC);

                memset(ports_bf, 0, sizeof(ports_bf));
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    VTSS_PORT_FORBID_BF_SET(ports_bf, pit.iport, port_registration[pit.iport]);
                }
                port_count = ((port_isid_port_count(sid))+3)/4;
                for(pb_index = 0; pb_index < port_count; pb_index++) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/",ports_bf[pb_index]);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                number_of_entries_left --;
            } /* if (vlan_mgmt_vlan_get(sid, vid_list, &conf, 0, VLAN_USER_STATIC, VLAN_MIXED_OP) == VTSS_OK) */
            vid_list++;
        } /* while(number_of_entries_left > 0 && vid_list <= VLAN_MAX) */
        send_all_vlans_conf(p);
        cyg_httpd_end_chunked();

        // Store the number of enteries shown per web page and the starting vid.
        prev_number_of_entries = number_of_entries;
        prev_start_vid = start_vid;
    }
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t vlan_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[VLAN_WEB_BUF_LEN];
    (void) snprintf(buff, VLAN_WEB_BUF_LEN, 
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            "var configHasIngressFiltering = 1;\n"
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
            "var configVlanIdMin = %d;\n" 
            "var configVlanIdMax = %d;\n",
             VLAN_ID_START, /* First VLAN ID */
             VLAN_ID_END   /* Last VLAN ID (entry number, not absolute ID) */
             );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(vlan_lib_config_js);

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t vlan_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[VLAN_WEB_BUF_LEN];
    (void) snprintf(buff, VLAN_WEB_BUF_LEN,
#ifndef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            ".haveIngressFiltering { display: none; }\r\n"
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
#ifndef VTSS_SW_OPTION_VLAN_NAME
            ".haveVLANName { display: none; }\r\n"
#endif  /* VTSS_SW_OPTION_VLAN_NAME */
#ifndef VTSS_SW_OPTION_MVRP
            ".has_mvrp { display: none; }\r\n"
#endif
#ifndef VTSS_FEATURE_VLAN_PORT_V2
            ".hasTPID { display: none; }\r\n"
            ".IsV2 { display: none; }\r\n"
#endif
#ifndef VTSS_FEATURE_VLAN_PORT_V1
            ".IsV1 { display: none; }\r\n"
#endif
            );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}
/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(vlan_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan_port, "/config/vlan_port", handler_config_vlan_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan, "/config/vlan", handler_config_vlan);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_vlan_membership, "/stat/vlan_membership_stat", handler_vlan_membership_stat);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_vlan_port, "/stat/vlan_port_stat", handler_vlan_ports_stat);

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
