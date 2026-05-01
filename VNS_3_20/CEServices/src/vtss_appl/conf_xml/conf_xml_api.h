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

 $Id$
 $Revision$

*/

#ifndef _VTSS_CONF_XML_API_H_
#define _VTSS_CONF_XML_API_H_

#include "main.h"
#include "conf_xml_struct.h"
/* Error codes (vtss_rc) */
typedef enum {
    CONF_XML_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_CONF_XML), /* Generic error code */
    CONF_XML_ERROR_FILE_SYNTAX, /* Error in file syntax */
    CONF_XML_ERROR_FILE_PARM,   /* Error in file parameter */
} conf_xml_error_t;

/* Maximum configuration file size. The worst case configuration currently
   looks like this:

   Global section (ACL: 1024*500, QCL: 24*26*120, Misc: 10*1024):   600 kB
   Switch sections (tested by disabling port lists)             : 16*32 kB
   -----------------------------------------------------------------------
   Total                                                           1112 kB

   For a number of reasons, the configuration will never actually reach this
   size. On the other hand, the number below may need an update if ACL/QCL
   configuration sizes are changed or new features with significant
   configuration are added.

   With exhaustive configuration of all the modules we have seen around 1MB
   of XML file for a single switch; based on this observation, we define
   the Max size as below.
*/
#if VTSS_SWITCH_STACKABLE
#define CONF_XML_FILE_MAX (16*1024*1024)
#else
#define CONF_XML_FILE_MAX (2*1024*1024)
#endif

/* File get */
typedef struct {
    char  *data; /* Input : Data pointer */
    ulong size;  /* Input : Maximum data size
                    Output: Data size */
} conf_xml_get_t;

/* Get current configuration to buffer */
vtss_rc conf_xml_file_get(conf_xml_get_t *file);

/* File set */
typedef struct {
    char  *data;     /* Input : Data pointer */
    ulong size;      /* Input : Data size */
    BOOL  apply;     /* Input : Apply file */
    ulong line_no;   /* Output: Error line number */
    char  line[256]; /* Output: Error line text */
    char  msg[256];  /* Output: Error messsage */
} conf_xml_set_t;

/* Set current configuration from buffer */
vtss_rc conf_xml_file_set(conf_xml_set_t *file);

/* Initialize XML module */
vtss_rc conf_xml_init(vtss_init_data_t *data);

/* Rest of the section defines structure/enum and utility functions
   provided by conf_xml module to control modules for XML support
*/
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_tables.h>





#if defined(VTSS_SW_OPTION_IP2)
#include "ip2_api.h"
#endif /*defined(VTSS_SW_OPTION_IP2)*/

/*
   Instruction to support XML for a Module:
   ========================================
   1. Define module's TAG IDs, this includes module, group and
      parameter tag
   2. Define the Tag table describing each tag name, description
      and tag type
   3. Define module specific set_state_t if the module needs to store
      specific parameter while parsing the XML file and want to save
      or check the config after finishing the section (i.e. global/switch)
   4. Write the gen_func() function which will write module config
      into XML file. gen_func will have global and switch section
   5. Write parse_func() function which will be called to parse module
      parameter, handle global and switch section data. Inter-module
      dependency check is handled inside cx_check_switch() in conf_xml.c.
      So if a module wants to check other module configuration, add that
      check inside cx_check_switch()
   6. Central XML provides lot of utility functions to control modules to
      build and parse paramter, like cx_add_tag_line, cx_add_stx_start,
      cx_add_stx_end, cx_add_attr_ipv4, etc for building the XML file;
      cx_parse_attr, cx_parse_txt etc for parsing parameter
   7. If a module needs to initialize it's XML data structure during
      system startup, it can define it's init_func()
   8. Finally register module's tag table, size of module set_state_t
      structure, init_func, gen_func and parse_func into central XML
      module by using the macro CX_MODULE_TAB_ENTRY

    For more information see the Conf_XML Design Spec: DS0225 to know
    tag type, global and switch section etc. Appendix A includes sample
    code for IP module which explains minimum data structure and function
    required for a module to support XML.
*/

/* Tag type */
typedef enum {
    CX_TAG_TYPE_HEADER,
    CX_TAG_TYPE_SECTION,
    CX_TAG_TYPE_MODULE,
    CX_TAG_TYPE_GROUP,
    CX_TAG_TYPE_PARM,
    CX_TAG_TYPE_NONE,
} cx_tag_type_t;

/* Tag entry */
typedef struct {
    char           *name;  /* Tag name */
    char           *descr; /* Tag description */
    cx_tag_type_t  type;   /* Tag type */
} cx_tag_entry_t;

typedef struct _cx_get_state_t cx_get_state_t;
typedef struct _cx_set_state_t cx_set_state_t;


/* module XML initialization routine prototype */
typedef vtss_rc cx_init_func(void);

/* XML file generation function prototype */
typedef vtss_rc cx_gen_func(cx_get_state_t *s);

/* XML parameter parsing and set routine prototype */
typedef vtss_rc cx_parse_func(cx_set_state_t *s);

/* Module entry */
struct cx_module_entry_t {
    vtss_module_id_t module; /* module ID */
    cx_tag_entry_t   *tag; /* module tag table pointer */
    size_t           size; /* block size for module specific config */
    u32              offset; /* offset from the end of vtss_cx_set_state_t */
    cx_init_func     *init_func; /* XML init routine */
    cx_gen_func      *gen_func; /* XML file generation routine */
    cx_parse_func    *parse_func; /* XML parsing and set function */
} CYG_HAL_TABLE_TYPE;

/* Entry definition rule for dynamically created XML module table */
#define CX_MODULE_TAB_ENTRY_DECL(_m_) \
    struct cx_module_entry_t  _cx_mod_tab_##_m_  \
    CYG_HAL_TABLE_QUALIFIED_ENTRY(cx_module_table, _m_)

#define CX_MODULE_TAB_ENTRY(_m_,_t_,_s_,_o_,_i_,_g_,_p_) \
    CX_MODULE_TAB_ENTRY_DECL(_m_) = {_m_,_t_,_s_,_o_,_i_,_g_,_p_}

/* XML gen command */
typedef enum {
    CX_GEN_CMD_GLOBAL, /* Configuration generation for global section */
    CX_GEN_CMD_SWITCH, /* Configuration generation for switch section */
} cx_gen_cmd_t;

/* File get state */
struct _cx_get_state_t {
    /* Conf_XML private */
    char   *file;  /* File pointer */
    ulong  size;   /* File size */
    char   *p;     /* Current file pointer */
    ulong  indent; /* Number of characters to indent */
    BOOL   error;  /* File exceeded error */

    /* Public */
    vtss_isid_t    isid;
    u32            port_count; /* Number of ports for ISID */
    cx_gen_cmd_t   cmd;  /* Section identifier */
    cx_tag_entry_t *tag; /* module specific tag table pointer */
};

/* File parse command */
typedef enum {
    CX_PARSE_CMD_PARM,   /* Parse parameter */
    CX_PARSE_CMD_GLOBAL, /* Parse global section */
    CX_PARSE_CMD_SWITCH, /* Parse switch section */
} cx_parse_cmd_t;

/* Line number information */
typedef struct {
    ulong number; /* Line number */
    char  *txt;   /* Line string */
} cx_line_t;

typedef u32 cx_tag_id_t;

/* Maximum number of tags in stack */
#define CX_TAG_MAX 10

/* File set state */
struct _cx_set_state_t {
    /* Tag stack */
    struct {
        vtss_module_id_t  module;
        cx_tag_id_t       id;
        cx_tag_type_t     type;
    } tag[CX_TAG_MAX]; /* Tag information */
    int   tag_count; /* Tag count */

    BOOL  sid_done[VTSS_USID_END];
    BOOL  master_elect;  /* Stack master reelection */

    /* Public */
    vtss_rc     rc;     /* Error code */
    cx_line_t   line;   /* Current line */
    char        *msg;   /* Error message */
    char        *p;     /* Current file pointer */
    vtss_isid_t isid;   /* Current section */
    u32         port_count; /* Number of ports for current ISID */

    struct cx_module_entry_t *mod_tab; /* Current module table entry */

    vtss_module_id_t  mod_id;  /* Current module ID */
    cx_tag_id_t       mod_tag; /* Current module tag, requires as some modules
                                  have more than one module tag */
    cx_tag_id_t       group;  /* Current group tag */
    cx_tag_id_t       id;     /* Current parm tag */
    cx_tag_type_t     tag_type; /* To know the type of Tag */
    BOOL              ignored; /* tag ignored by module */

    char       *name;     /* Attribute name start */
    int         name_len;  /* Attribute name length */
    char       *val;      /* Attribute value start */
    int         val_len;   /* Attribute value length */
    char       *next;     /* Next character after attribute */

    cx_parse_cmd_t cmd;  /* type of parsing */
    BOOL           apply;  /* configuration set */
    BOOL           init;  /* configuration initialization */
    void           *mod_state;  /* module specific config */
};

/* Table enumerators */
typedef struct {
    vtss_isid_t isid;     /* Always present */
    void        *custom;  /* Optional per table */
} cx_table_context_t;

typedef BOOL (*cx_table_entry_match_t)(const cx_table_context_t *context,
                                       ulong idx_a,
                                       ulong idx_b);

typedef vtss_rc (*cx_table_entry_print_t)(cx_get_state_t *s,
                                          const cx_table_context_t *context,
                                          ulong idx,
                                          char *txt);

/* Two bytes encoded in one value */
#define CX_DUAL_VAL(v1, v2) ((v1<<8) | v2)
#define CX_DUAL_V1(val)     ((val>>8) & 0xff)
#define CX_DUAL_V2(val)     (val & 0xff)

/* Keyword match type */
typedef struct {
    char  *name;
    ulong val;
} cx_kw_t;

#define CX_RC(expr) { vtss_rc _rc_ = (expr); if (_rc_ != VTSS_OK) return _rc_; }

/* gen utility function prototype */
vtss_rc cx_size_check(cx_get_state_t *s);
vtss_rc cx_add_tag_line(cx_get_state_t *s, cx_tag_id_t id, BOOL end);
vtss_rc cx_add_attr_start(cx_get_state_t *s, cx_tag_id_t id);
vtss_rc cx_add_attr_txt(cx_get_state_t *s, const char *name, const char *txt);
vtss_rc cx_add_stx_start(cx_get_state_t *s);
vtss_rc cx_add_stx_end(cx_get_state_t *s);
vtss_rc cx_add_comment(cx_get_state_t *s, const char *txt);
vtss_rc cx_add_stx_txt(cx_get_state_t *s, const char *txt);
vtss_rc cx_add_stx_ipv4(cx_get_state_t *s, const char *name);
vtss_rc cx_add_stx_ipv6(cx_get_state_t *s, const char *name);
vtss_rc cx_add_stx_flag(cx_get_state_t *s, const char *name);
vtss_rc cx_add_attr_hex(cx_get_state_t *s, const char *name, ulong val);
vtss_rc cx_add_attr_ulong(cx_get_state_t *s, const char *name, ulong val);
vtss_rc cx_add_attr_ulong_word(cx_get_state_t *s, const char *name, ulong val,
                               const char *word, ulong match);
vtss_rc cx_add_attr_long(cx_get_state_t *s, const char *name, long val);
vtss_rc cx_add_attr_kw(cx_get_state_t *s, const char *name,
                       const cx_kw_t *keyword, ulong val);
vtss_rc cx_add_attr_bool(cx_get_state_t *s, const char *name, BOOL enable);
vtss_rc cx_add_stx_kw(cx_get_state_t *s, const char *name, const cx_kw_t *keyword);
vtss_rc cx_add_stx_bool(cx_get_state_t *s, const char *name);
vtss_rc cx_add_stx_ulong(cx_get_state_t *s, const char *name, ulong min, ulong max);
vtss_rc cx_add_stx_ulong_word(cx_get_state_t *s, const char *name,
                              const char *word, ulong min, ulong max);
char *cx_list_txt(char *buf, ulong min, ulong max);
vtss_rc cx_add_stx_port(cx_get_state_t *s);
vtss_rc cx_add_attr_ipv4(cx_get_state_t *s, const char *name, vtss_ipv4_t ipv4);
vtss_rc cx_add_attr_ipv6(cx_get_state_t *s, const char *name, vtss_ipv6_t ipv6);
vtss_rc cx_add_val_ipv4(cx_get_state_t *s, cx_tag_id_t id, vtss_ipv4_t ipv4);
vtss_rc cx_add_val_ipv6(cx_get_state_t *s, cx_tag_id_t id, vtss_ipv6_t ipv6);

vtss_rc cx_add_attr_end(cx_get_state_t *s, cx_tag_id_t id);
vtss_rc cx_add_val_ulong_stx(cx_get_state_t *s,
                             cx_tag_id_t id, ulong val, const char *stx);
vtss_rc cx_add_val_ulong(cx_get_state_t *s, cx_tag_id_t id, ulong val,
                         ulong min, ulong max);
vtss_rc cx_add_val_long(cx_get_state_t *s,
                        cx_tag_id_t id, long val, const char *stx);
vtss_rc cx_add_val_txt(cx_get_state_t *s, cx_tag_id_t id,
                       const char *txt, const char *stx);
vtss_rc cx_add_val_kw(cx_get_state_t *s, cx_tag_id_t id,
                      const cx_kw_t *keyword, ulong val);
vtss_rc cx_add_port_start(cx_get_state_t *s, cx_tag_id_t id, const char *ports);
vtss_rc cx_add_port_end(cx_get_state_t *s, cx_tag_id_t id);
vtss_rc cx_add_val_bool(cx_get_state_t *s, cx_tag_id_t id, BOOL enable);
vtss_rc cx_add_table(cx_get_state_t *s,
                     const cx_table_context_t *context,
                     ulong min, ulong max,
                     BOOL entry_skip(ulong idx),
                     cx_table_entry_match_t entry_match,
                     cx_table_entry_print_t entry_print,
                     BOOL port_list);
vtss_rc cx_add_port_table_ex(cx_get_state_t *s,
                             vtss_isid_t isid,
                             void *custom_context,
                             cx_tag_id_t tag,
                             cx_table_entry_match_t port_match,
                             cx_table_entry_print_t port_print);
vtss_rc cx_add_port_table(cx_get_state_t *s,
                          vtss_isid_t isid,
                          cx_tag_id_t tag,
                          cx_table_entry_match_t port_match,
                          cx_table_entry_print_t port_print);

/* parse utility function prototype */
vtss_rc cx_parm_invalid(cx_set_state_t *s);
vtss_rc cx_parm_unknown(cx_set_state_t *s);
vtss_rc cx_parm_error(cx_set_state_t *s, const char *txt);
vtss_rc cx_parm_found_error(cx_set_state_t *s, const char *txt);
vtss_rc cx_parse_attr(cx_set_state_t *s);
vtss_rc cx_parse_attr_name(cx_set_state_t *s, const char *name);
vtss_rc cx_parse_txt(cx_set_state_t *s, const char *name, char *buf, int max);
BOOL cx_word_match(const char *stx, const char *word);
vtss_rc cx_parse_word(cx_set_state_t *s, const char *name, const char *word);
vtss_rc cx_parse_ulong_word(cx_set_state_t *s, const char *name, ulong *val,
                            ulong min, ulong max, const char *word, ulong match);
vtss_rc cx_parse_ulong(cx_set_state_t *s, const char *name,
                       ulong *val, ulong min, ulong max);
vtss_rc cx_parse_ulong_dis(cx_set_state_t *s, const char *name, ulong *val,
                           ulong min, ulong max, ulong match);
vtss_rc cx_parse_val_ulong(cx_set_state_t *s, ulong *val,
                           ulong min, ulong max);
vtss_rc cx_parse_long(cx_set_state_t *s, const char *name,
                      long *val, long min, long max);
vtss_rc cx_parse_val_long(cx_set_state_t *s, long *val, long min, long max);
vtss_rc cx_parse_val_txt(cx_set_state_t *s, char *buf, int max);
vtss_rc cx_parse_val_txt_numbers_only(cx_set_state_t *s, char *buf, int max);
vtss_rc cx_parse_kw(cx_set_state_t *s, const char *name, const cx_kw_t *keyword,
                    ulong *val, BOOL force);
vtss_rc cx_parse_val_kw(cx_set_state_t *s, const cx_kw_t *keyword,
                        ulong *val, BOOL force);
vtss_rc cx_parse_bool(cx_set_state_t *s, const char *name, BOOL *val, BOOL force);
vtss_rc cx_parse_val_bool(cx_set_state_t *s, BOOL *val, BOOL force);
vtss_rc cx_parse_ports_name(cx_set_state_t *s, BOOL *list, BOOL force, char *name);
vtss_rc cx_parse_ports(cx_set_state_t *s, BOOL *list, BOOL force);
vtss_rc cx_parse_vid(cx_set_state_t *s, vtss_vid_t *vid, BOOL force);
vtss_rc cx_parse_ipv4(cx_set_state_t *s, const char *name, vtss_ipv4_t *ipv4, vtss_ipv4_t *mask, BOOL is_mask);
vtss_rc cx_parse_ipmcv4(cx_set_state_t *s, const char *name, vtss_ipv4_t *ipv4);
vtss_rc cx_parse_val_ipv4(cx_set_state_t *s, vtss_ipv4_t *ipv4, vtss_ipv4_t *mask, BOOL is_mask);

vtss_rc cx_parse_ipv6(cx_set_state_t *s, const char *name, vtss_ipv6_t *ipv6);
vtss_rc cx_parse_ipmcv6(cx_set_state_t *s, const char *name, vtss_ipv6_t *ipv6);
vtss_rc cx_parse_val_ipv6(cx_set_state_t *s, vtss_ipv6_t *ipv6);

vtss_rc cx_parse_mac(cx_set_state_t *s, const char *name, uchar *mac);
vtss_rc cx_parse_any(cx_set_state_t *s, const char *name);
vtss_rc cx_parse_all(cx_set_state_t *s, const char *name);
vtss_rc cx_parse_udp_tcp(cx_set_state_t *s, const char *name,
                         vtss_udp_tcp_t *min, vtss_udp_tcp_t *max);
#ifdef VTSS_SW_OPTION_QOS
vtss_rc cx_parse_class(cx_set_state_t *s, const char *name, vtss_prio_t *prio);
#endif /* VTSS_SW_OPTION_QOS */

/**
 * \brief Sets the hex_string size calculated from a given data value
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size value will be used to calculate the string size
 *			*config conf->hex_string.size will be set by the resulting value
 * \return
 */
void struct_to_string_size_cal(config_transport_t * conf);

/**
 * \brief Takes a data structure and converts it to a hexadecimal string
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.pointer (Input)points to the structure to be converted
 * 			*config conf->data.size (Input) indicates the size of the structure
 * 			*config conf->hex_string.pointer (Output)gives location to insert the converted values
 * 			*config conf->hex_string.size (Input) gives the size of the buffer the string is being converted to
 * \return 0 when successful, -1 if a NULL parameter is given, -2 if hex_string buffer size is not sufficient
 */
int struct_to_string(const config_transport_t * conf);

/**
 * \brief Takes a hexadecimal string and convert it to a unsigned char array or structure
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->hex_string.pointer (Input) gives location to inserted converted values
 * 			*config conf->hex_string.size (Input)gives the size of the buffer the string is being converted to. Needs to be set by struct_to_string_size_cal
 * 			*config conf->data.pointer (Output)points to the structure to be converted
 * 			*config conf->data.size (Input) indicates the size of the structure. Needs to be set by modules get_module_master_conf_size
 * \return 0 when successful, -1 if a NULL parameter is given, -2 if hex_string buffer size is not sufficient
 */
int hexstring_to_struct(const config_transport_t * conf );


#endif /* _VTSS_CONF_XML_API_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
