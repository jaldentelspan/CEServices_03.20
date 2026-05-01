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

#include <linux/module.h>  /* can't do without it */
#include <linux/version.h> /* and this too */

#include <linux/proc_fs.h>
#include <linux/ioport.h>

#include <linux/io.h>
#include <linux/semaphore.h>

#include <linux/uaccess.h>

#include "vtss_switch.h"

#define DEBUG
int debug = false, debug_current = false;

DECLARE_MUTEX(apisem);
struct proc_dir_entry *switch_proc_dir;
static struct proc_dir_entry *switch_proc_stats;
static struct proc_dir_entry *switch_proc_mactable;
EXPORT_SYMBOL(switch_proc_dir);

const port_custom_entry_t *port_custom_table;
EXPORT_SYMBOL( port_custom_table );

static vtss_inst_t chipset;

/* Board information */
static vtss_board_info_t board_info;

static void update_debug(void)
{
    vtss_trace_conf_t tconf;
    vtss_trace_group_t group;
    vtss_trace_level_t level = (debug ? VTSS_TRACE_LEVEL_DEBUG : VTSS_TRACE_LEVEL_ERROR);

    for (group = 0; group < VTSS_TRACE_GROUP_COUNT; group++) {
        vtss_trace_conf_get(group, &tconf);
        tconf.level[VTSS_TRACE_LAYER_AIL] = level;
        tconf.level[VTSS_TRACE_LAYER_CIL] = level;
        vtss_trace_conf_set(group, &tconf);
    }

    debug_current = debug;
}

/******************************************************************************/
/******************************************************************************/

static int i2uport(vtss_port_no_t port_no)
{
    return (int) (port_no+1);
}

static int SWITCH_proc_read_stats(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    vtss_port_counters_t               counters;
    vtss_port_rmon_counters_t          *rmon = &counters.rmon;
    vtss_port_no_t port_no;
    int bytes_written = 0;

    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
        if (vtss_port_counters_get(chipset, port_no, &counters) == VTSS_RC_OK)
            bytes_written += snprintf(buf + bytes_written, count - bytes_written - 1, 
                                      "%2d %19llu %19llu\n", 
                                      i2uport(port_no), rmon->rx_etherStatsPkts, rmon->tx_etherStatsPkts);
    }

    return bytes_written;
}

static char *port_list_txt(BOOL port_list[VTSS_PORT_ARRAY_SIZE])
{
    static char    buf[64];
    char           *p = buf;
    vtss_port_no_t port_no, port;
    int            first = 1, count = 0;
    BOOL           member;

    *p = '\0';
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
        member = port_list[port_no];
        if ((member && 
             (count == 0 || port_no == (VTSS_PORTS - 1))) || (!member && count > 1)) {
            port = i2uport(port_no);
            p += sprintf(p, "%s%u",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         member ? port : (port - 1));
            first = 0;
        }
        if (member)
            count++;
        else
            count=0;
    }
    if (p == buf)
        sprintf(p, "None");
    return buf;
}

static char *mac_txt(const u8 *mac)
{
    static char buf[64];
    snprintf(buf, sizeof(buf), 
             "%02x-%02x-%02x-%02x-%02x-%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

static int SWITCH_proc_read_mactable(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    vtss_mac_table_entry_t entry;
    int bytes_written = 0;

    memset(&entry, 0, sizeof(entry));
    while (vtss_mac_table_get_next(chipset, &entry.vid_mac, &entry) == VTSS_RC_OK) {
        bytes_written += snprintf(buf + bytes_written, count - bytes_written - 1, "%-4d %s %s%s\n",
                                  entry.vid_mac.vid, 
                                  mac_txt(entry.vid_mac.mac.addr), 
                                  port_list_txt(entry.destination),
                                  entry.copy_to_cpu ? ",CPU" : "");
    }

    return bytes_written;
}

/* Trace hex-dump callout function */
void vtss_callout_trace_hex_dump(const vtss_trace_layer_t layer,
                                 const vtss_trace_group_t group,
                                 const vtss_trace_level_t level,
                                 const char               *file,
                                 const int                line,
                                 const char               *function,
                                 const unsigned char      *byte_p,
                                 const int                byte_cnt)
{
    // To be implemented.
}

/* Trace callout function */
void vtss_callout_trace_printf(const vtss_trace_layer_t  layer,
                               const vtss_trace_group_t  group,
                               const vtss_trace_level_t  level,
                               const char                *file,
                               const int                 line,
                               const char                *function,
                               const char                *format,
                               ...)
{
    va_list args;
    u_char tmpbuf[132], *p;
    
    p = &tmpbuf[0];
    p += sprintf(p, "%s%s:%d ",
                 level == VTSS_TRACE_LEVEL_ERROR ? KERN_WARNING :
                 level == VTSS_TRACE_LEVEL_INFO ?  KERN_NOTICE :
                 level == VTSS_TRACE_LEVEL_DEBUG ? KERN_INFO :
                 KERN_DEBUG,
                 function, line);

    va_start(args, format);
    p += vsnprintf(p, sizeof(tmpbuf) - (p-tmpbuf) - 2, format, args);
    va_end(args);
    *p++ = '\n';
    *p = 0;
    printk(tmpbuf);
}

void vtss_callout_lock(const char *function)
{
    down(&apisem);
}

void vtss_callout_unlock(const char *function)
{
    up(&apisem);
}

#define VTSS_IO_ORIGIN1_REL_WTOP    (VTSS_IO_ORIGIN1_SIZE >> 2)
#define VTSS_IO_ORIGIN2_REL_WOFFSET ((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) >> 2)
#define VTSS_IO_ORIGIN2_REL_WTOP    (((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) + VTSS_IO_ORIGIN2_SIZE) >> 2)
void __iomem *relreg2virt(u32 addr)
{
    if(addr < VTSS_IO_ORIGIN1_REL_WTOP) {
        u32 __iomem *base = map_base_switch;
        return &base[addr];
    } else if(addr > VTSS_IO_ORIGIN2_REL_WOFFSET && addr < VTSS_IO_ORIGIN2_REL_WTOP) {
        u32 __iomem *base = map_base_cpu;
        addr -= VTSS_IO_ORIGIN2_REL_WOFFSET;
        return &base[addr];
    } else {
        printk(KERN_ERR "switch: Invalid register access, offset 0x%08x\n", addr);
        BUG();
    }
    return NULL;
}

/* Memory mapped IO 1:1 */
static vtss_rc reg_read(const vtss_chip_no_t chip_no, /* ARGSUSED */
                        const u32            addr,
                        u32                  *const value)
{
#if defined(CONFIG_VTSS_VCOREIII_JAGUAR_DUAL)
    if (chip_no == 0)
        *value = readl(relreg2virt(addr));
    else {
        u32 val = 0;
        if(addr < VTSS_IO_ORIGIN1_REL_WTOP) {
            val = readl(&((volatile u32 __iomem *)map_base_slv_switch)[addr]);
        } else {
            printk(KERN_ERR "switch: Invalid register access, offset 0x%08x, chip %d\n", addr, chip_no);
            BUG();
        }
        *value = val;
    }
#else
    *value = readl(relreg2virt(addr));
#endif
    return VTSS_RC_OK;
}

static vtss_rc reg_write(const vtss_chip_no_t chip_no, /* ARGSUSED */
                         const u32            addr,
                         const u32            value)
{
#if defined(CONFIG_VTSS_VCOREIII_JAGUAR_DUAL)
    if (chip_no == 0)
        writel(value, relreg2virt(addr));
    else {
        if(addr < VTSS_IO_ORIGIN1_REL_WTOP) {
            writel(value, &((volatile u32 __iomem*)map_base_slv_switch)[addr]);
        } else {
            printk(KERN_ERR "switch: Invalid register access, offset 0x%08x, chip %d\n", addr, chip_no);
            BUG();
        }
    }
#else
    writel(value, relreg2virt(addr));
#endif
    return VTSS_RC_OK;
}

static unsigned int __init vtss_api_chipid(void)
{
    /* The cumbersome mapping of VTSS_CHIP_XXX to VTSS_TARGET_XXX */
#if defined(VTSS_CHIP_E_STAX_34)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_E_STAX_34
#elif defined(VTSS_CHIP_SPARX_II_16)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_II_16
#elif defined(VTSS_CHIP_SPARX_II_24)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_II_24
#elif defined(VTSS_CHIP_SPARX_III_10)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_III_10
#elif defined(VTSS_CHIP_SPARX_III_10_01)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_III_10_01
#elif defined(VTSS_CHIP_SPARX_III_18)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_III_18
#elif defined(VTSS_CHIP_SPARX_III_24)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_III_24
#elif defined(VTSS_CHIP_SPARX_III_26)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SPARX_III_26
#elif defined(VTSS_CHIP_CARACAL_1)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_CARACAL_1
#elif defined(VTSS_CHIP_CARACAL_2)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_CARACAL_2
#elif defined(VTSS_CHIP_JAGUAR_1)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_JAGUAR_1
#elif defined(VTSS_CHIP_LYNX_1)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_LYNX_1
#elif defined(VTSS_CHIP_CE_MAX_24)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_CE_MAX_24
#elif defined(VTSS_CHIP_CE_MAX_12)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_CE_MAX_12
#elif defined(VTSS_CHIP_E_STAX_III_48_DUAL)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_E_STAX_III_48_DUAL
#elif defined(VTSS_CHIP_E_STAX_III_68_DUAL)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_E_STAX_III_68_DUAL
#elif defined(VTSS_CHIP_JAGUAR_1_DUAL)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_JAGUAR_1_DUAL
#elif defined(VTSS_CHIP_SERVAL)
#define VTSS_SWITCH_API_TARGET VTSS_TARGET_SERVAL
#else
#error Unable to determine Switch API target - check VTSS_CHIP_XXX define!
#endif
    return VTSS_SWITCH_API_TARGET;
}

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26)
#define VTSS_SW_OPTION_BOARD_PROBE_PRE_INIT
#endif /* VTSS_ARCH_JAGUAR_1 || VTSS_ARCH_LUTON26 */

static int __init vtss_switch_init(void)
{
    vtss_inst_create_t      create;
    vtss_init_conf_t        conf;

    init_MUTEX(&apisem);

    if(debug)
        update_debug();

    board_info.target = vtss_api_chipid();
    if(board_info.target == VTSS_TARGET_E_STAX_III_24_DUAL)
        board_info.board_type = VTSS_BOARD_JAG_CU48_REF;
    else
        board_info.board_type = VTSS_BOARD_UNKNOWN;
    board_info.port_count = VTSS_PORTS;
    board_info.reg_read = reg_read;
    board_info.reg_write = reg_write;

#if defined(VTSS_SW_OPTION_BOARD_PROBE_PRE_INIT)
    /* Detect board type before API is initialized */
    vtss_board_probe(&board_info);
    VTSS_ASSERT(port_custom_table != NULL);
#endif /* VTSS_SW_OPTION_BOARD_PROBE_PRE_INIT */

    /* Instantiate chip(s) */
    vtss_inst_get(board_info.target, &create);
    if(vtss_inst_create(&create, &chipset) != VTSS_RC_OK)
        return -ENXIO;
        
    /* Setup register interface */
    vtss_init_conf_get(chipset, &conf);
    conf.reg_read = board_info.reg_read;
    conf.reg_write = board_info.reg_write;
    vtss_init_conf_set(chipset, &conf);

#if !defined(VTSS_SW_OPTION_BOARD_PROBE_PRE_INIT)
    /* Detect board type after API is initialized */
    vtss_board_probe(&board_info);
    VTSS_ASSERT(port_custom_table != NULL);
#endif /* VTSS_SW_OPTION_BOARD_PROBE_PRE_INIT */

    printk(KERN_INFO "switch: '%s' board detected\n", vtss_board_name());

    switch_proc_stats = NULL;
    switch_proc_mactable = NULL;

    // Create the /proc/switch file system entries
    switch_proc_dir = proc_mkdir("switch", NULL);
    if(switch_proc_dir) {
        if((switch_proc_stats = create_proc_entry("counters", S_IFREG | S_IRUGO, switch_proc_dir)))
            switch_proc_stats->read_proc = SWITCH_proc_read_stats;
        if((switch_proc_mactable = create_proc_entry("mactable", S_IFREG | S_IRUGO, switch_proc_dir)))
            switch_proc_mactable->read_proc = SWITCH_proc_read_mactable;
    }

    return 0;
}

static void vtss_switch_exit(void)
{

    // Delete the /proc entries
    if(switch_proc_stats) {
        remove_proc_entry(switch_proc_stats->name, switch_proc_stats->parent);
        switch_proc_stats = NULL;
    }
    if(switch_proc_mactable) {
        remove_proc_entry(switch_proc_mactable->name, switch_proc_mactable->parent);
        switch_proc_mactable = NULL;
    }
    if(switch_proc_dir) {
        remove_proc_entry(switch_proc_dir->name, switch_proc_dir->parent);
        switch_proc_dir = NULL;
    }

    printk(KERN_NOTICE "Uninstalled\n");
}

EXPORT_SYMBOL( port_custom_reset );
EXPORT_SYMBOL( post_port_custom_reset );
EXPORT_SYMBOL( port_custom_init );
EXPORT_SYMBOL( port_custom_conf );
EXPORT_SYMBOL( port_custom_led_init );
EXPORT_SYMBOL( port_custom_led_update );

EXPORT_SYMBOL( vtss_board_name );
EXPORT_SYMBOL( vtss_board_type );
EXPORT_SYMBOL( vtss_board_features );
EXPORT_SYMBOL( vtss_board_chipcount );
EXPORT_SYMBOL( vtss_board_port_cap );

module_init(vtss_switch_init);
module_exit(vtss_switch_exit);

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable debug");

MODULE_AUTHOR("Lars Povlsen <lpovlsen@vitesse.com>");
MODULE_DESCRIPTION("Vitesse Gigabit Switch Core Module");
MODULE_LICENSE("(c) Vitesse Semiconductor Inc.");
