/*

 Vitesse Switch API software.

 Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
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
#include "vtss_types.h"
#include "tod.h"
#include "tod_api.h"
#include "vtss_tod_api.h"
#include "critd_api.h"
#include "conf_api.h"
#include "interrupt_api.h"      /* interrupt handling */
#if defined (VTSS_ARCH_SERVAL)
#include "port_api.h"
#endif /* VTSS_ARCH_SERVAL */

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
#include "vtss_tod_mod_man.h"
#ifdef VTSS_SW_OPTION_VCLI
#include "tod_cli.h"
#endif
#endif

//#define CALC_TEST

#ifdef CALC_TEST
#include "vtss_ptp_types.h"
#endif

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */


#if (VTSS_TRACE_ENABLED)

static vtss_trace_reg_t trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "tod",
    .descr     = "Time of day for PTP and OAM etc."
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default (TOD core)",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
        .usec = 1,
    },
    [VTSS_TRACE_GRP_CLOCK] = {
        .name      = "clock",
        .descr     = "TOD time functions",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
    [VTSS_TRACE_GRP_MOD_MAN] = {
        .name      = "mod_man",
        .descr     = "PHY Timestamp module manager",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [VTSS_TRACE_GRP_PHY_TS] = {
        .name      = "phy_ts",
        .descr     = "PHY Timestamp feature",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [VTSS_TRACE_GRP_REM_PHY] = {
        .name      = "rem_phy",
        .descr     = "PHY Remote Timestamp feature",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
#endif        
};
#endif /* VTSS_TRACE_ENABLED */

static struct {
    BOOL ready;                 /* TOD Initited  */
    critd_t datamutex;          /* Global data protection */
} tod_global;


/* ================================================================= *
 *  Configuration definitions
 * ================================================================= */

typedef struct tod_config_t {
    i8 version;             /* configuration version, to be changed if this structure is changed */
    vtss_tod_internal_tc_mode_t int_mode;  /* internal timestamping mode */
#if defined (VTSS_ARCH_SERVAL)
    BOOL phy_ts_enable[VTSS_PORTS];  /* enable PHY timestamping mode */
#endif // (VTSS_ARCH_SERVAL)
} tod_config_t;

static tod_config_t config_data ;
/*
 * Propagate the PTP (module) configuration to the PTP core
 * library.
 */
static void
tod_conf_propagate(void)
{
    T_I("TOD Configuration has been reset, you need to reboot to activate the changed conf." );
}

static void
tod_conf_save(void)
{
    tod_config_t    *conf;  /* run-time configuration data */
    ulong size;
    if ((conf = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_TOD_CONF, &size)) != NULL) {
        if (size == sizeof(*conf)) {
            T_IG(0, "Saving configuration");
            TOD_DATA_LOCK();
            *conf = config_data;
            TOD_DATA_UNLOCK();
        }
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_TOD_CONF);
    }
}

/**
 * Read the PTP configuration.
 * \param create indicates a new default configuration block should be created.
 *
 */
static void
tod_conf_read(BOOL create)
{
    ulong           size;
    BOOL            do_create;
	BOOL            tod_conf_changed = FALSE;
    tod_config_t    *conf;  /* run-time configuration data */
#if defined (VTSS_ARCH_SERVAL) && !defined (VTSS_CHIP_SERVAL_LITE)
    int i;
#endif
    if ((conf = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_TOD_CONF, &size)) == NULL ||
            size != sizeof(*conf)) {
        conf = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_TOD_CONF, sizeof(*conf));
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        T_W("actual size = %d,expected size = %d", size, sizeof(*conf));
        do_create = TRUE;
    } else if (conf->version != TOD_CONF_VERSION) {
        T_W("version mismatch, creating defaults");
        do_create = TRUE;
    } else {
        do_create = create;
    }

    TOD_DATA_LOCK();
    if (do_create) {
		/* Check if configuration changed */
		if (config_data.int_mode != VTSS_TOD_INTERNAL_TC_MODE_30BIT) tod_conf_changed = TRUE;
        /* initialize run-time options to reasonable values */
        config_data.version = TOD_CONF_VERSION;
        config_data.int_mode = VTSS_TOD_INTERNAL_TC_MODE_30BIT;
#if defined (VTSS_ARCH_SERVAL) && !defined (VTSS_CHIP_SERVAL_LITE)
        for (i = 0; i < 4 && i < VTSS_PORTS; i++) {
			if (config_data.phy_ts_enable[i] != TRUE) tod_conf_changed = TRUE;
            config_data.phy_ts_enable[i] = TRUE;
        }
#endif
        *conf = config_data;
		if (tod_conf_changed) {
			T_W("TOD Configuration has been reset, you need to reboot to activate the changed conf." );
		}
		
    } else {
        config_data = *conf;
    }
    TOD_DATA_UNLOCK();

    if (conf) {
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_TOD_CONF);
    }

}
/****************************************************************************
 * Configuration API
 ****************************************************************************/

BOOL tod_tc_mode_get(vtss_tod_internal_tc_mode_t *mode)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    ok = TRUE;
    *mode = config_data.int_mode;
    TOD_DATA_UNLOCK();
    return ok;
}
BOOL tod_tc_mode_set(vtss_tod_internal_tc_mode_t *mode)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    if (*mode < VTSS_TOD_INTERNAL_TC_MODE_MAX) {
        ok = TRUE;
        config_data.int_mode = *mode;
    }
    TOD_DATA_UNLOCK();
    if (ok)
        tod_conf_save();
    return ok;
}

BOOL tod_port_phy_ts_get(BOOL *ts, vtss_port_no_t portnum)
{
    BOOL ok = FALSE;
    *ts = FALSE;
#if defined (VTSS_ARCH_SERVAL)
    TOD_DATA_LOCK();
    if (portnum < port_isid_port_count(VTSS_ISID_LOCAL)) {
        *ts = config_data.phy_ts_enable[portnum];
        ok = TRUE;
    }
    TOD_DATA_UNLOCK();
#endif /*defined (VTSS_ARCH_SERVAL) */
    return ok;
}

BOOL tod_port_phy_ts_set(BOOL *ts, vtss_port_no_t portnum)
{
    BOOL ok = FALSE;
#if defined (VTSS_ARCH_SERVAL)
    TOD_DATA_LOCK();
    if (portnum < port_isid_port_count(VTSS_ISID_LOCAL)) {
        config_data.phy_ts_enable[portnum] = *ts;
        ok = TRUE;
    }
    TOD_DATA_UNLOCK();
#endif /* defined (VTSS_ARCH_SERVAL) */
    if (ok)
        tod_conf_save();
    return ok;
}
BOOL tod_ready(void)
{
	return tod_global.ready;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/
#ifdef CALC_TEST

#define TC(t,c) ((t<<16) + c)
static void calcTest(void)
{
    u32 r, x, y;
    vtss_timeinterval_t t;
    char str1 [30];
    /* Ecos time counter = (10 ms ticks)<<16 + clocks , wraps at 1 sec*/

/* sub */    
    x = TC(43,4321);
    y = TC(41,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(43,4321);
    y = TC(41,4321); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,4321);
    y = TC(41,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,4321);
    y = TC(42,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* tick wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(43,4321);
    y = TC(41,4322); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* clk wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );

/* add */    
    x = TC(41,1234);
    y = TC(43,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8781); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8782); 
    vtss_tod_ts_cnt_add(&r, x, y); /* clk wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8783); 
    vtss_tod_ts_cnt_add(&r, x, y); /* clk wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(58,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(48,1234);
    y = TC(52,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* tick wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );


/* timeinterval to cnt */
    t = VTSS_SEC_NS_INTERVAL(0,123456);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,123456789);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,999999000);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,999999999);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(1,0);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );

/* cnt to timeinterval */
    x = TC(48,1234);
    vtss_tod_ts_cnt_to_timeinterval(&t, x);
    T_W("cnt %ld,%ld => TimeInterval: %s",r>>16,r & 0xffff , TimeIntervalToString (&t, str1, '.'));
    
}
#endif

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#if defined(VTSS_ARCH_SERVAL)
static vtss_ts_internal_fmt_t serval_internal(void)
{
    vtss_tod_internal_tc_mode_t mode;
    if (tod_tc_mode_get(&mode)) {
        T_D("Internal mode: %d", mode);
        switch (mode) {
            case VTSS_TOD_INTERNAL_TC_MODE_30BIT: return TS_INTERNAL_FMT_RESERVED_LEN_30BIT;
            case VTSS_TOD_INTERNAL_TC_MODE_32BIT: return TS_INTERNAL_FMT_RESERVED_LEN_32BIT;
            case VTSS_TOD_INTERNAL_TC_MODE_44BIT: return TS_INTERNAL_FMT_SUB_ADD_LEN_44BIT_CF62;
            case VTSS_TOD_INTERNAL_TC_MODE_48BIT: return TS_INTERNAL_FMT_RESERVED_LEN_48BIT_CF_0;
            default: return TS_INTERNAL_FMT_NONE;
        }
    } else {
        return TS_INTERNAL_FMT_NONE;
    }
}
#endif /* (VTSS_ARCH_SERVAL) */

/*
 * TOD Synchronization 1 pps pulse update handler
 */
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
static void one_pps_pulse_interrupt_handler(vtss_interrupt_source_t     source_id,
        u32                         instance_id)
{
    BOOL ongoing_adj;
    T_N("One sec pulse event: source_id %d, instance_id %u", source_id, instance_id);
    TOD_RC(vtss_ts_adjtimer_one_sec(0, &ongoing_adj));
    /* trig ModuleManager */
#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
    TOD_RC(ptp_module_man_trig(ongoing_adj));
#endif    
    TOD_RC(vtss_interrupt_source_hook_set(one_pps_pulse_interrupt_handler,
                                          INTERRUPT_SOURCE_SYNC,
                                          INTERRUPT_PRIORITY_NORMAL));
}
#endif




vtss_rc
tod_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
        critd_init(&tod_global.datamutex, "tod_data", VTSS_MODULE_ID_TOD, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        TOD_DATA_UNLOCK();
        //critd_init(&tod_global.coremutex, "tod_core", VTSS_MODULE_ID_TOD, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        //TOD_CORE_UNLOCK();
#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
        TOD_RC(ptp_module_man_init());
#ifdef VTSS_SW_OPTION_VCLI
        tod_cli_init();
#endif
#endif        
        T_I("INIT_CMD_INIT TOD" );
        break;
    case INIT_CMD_START:
        T_I("INIT_CMD_START TOD");
        break;
    case INIT_CMD_CONF_DEF:
        tod_conf_read(TRUE);
        tod_conf_propagate();
        T_I("INIT_CMD_CONF_DEF TOD" );
        break;
    case INIT_CMD_MASTER_UP:
        T_I("INIT_CMD_MASTER_UP ");
        tod_conf_read(FALSE);
#if defined(VTSS_ARCH_SERVAL)
        vtss_ts_internal_mode_t mode;
        mode.int_fmt = serval_internal();
        T_D("Internal mode.int_fmt: %d", mode.int_fmt);
        TOD_RC(vtss_ts_internal_mode_set(NULL, &mode));
#endif

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
        TOD_RC(ptp_module_man_resume());
#endif 
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        TOD_RC(vtss_interrupt_source_hook_set(one_pps_pulse_interrupt_handler,
                                              INTERRUPT_SOURCE_SYNC,
                                              INTERRUPT_PRIORITY_NORMAL));
#endif        
#ifdef CALC_TEST
        calcTest();
#endif
		tod_global.ready = TRUE;
        break;
    case INIT_CMD_SWITCH_ADD:
        T_I("INIT_CMD_SWITCH_ADD - ISID %u", isid);
        break;
    case INIT_CMD_MASTER_DOWN:
        T_I("INIT_CMD_MASTER_DOWN - ISID %u", isid);
        break;
    default:
        break;
    }

    return 0;
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
