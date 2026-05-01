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

// Avoid "vtss_api.h not used in module vtss_wis_api.c"
/*lint --e{766} */
#include "vtss_api.h"
#if defined(VTSS_FEATURE_WIS)
#include "vtss_state.h"

#define VTSS_PHY_EWIS_ASSERT(x) if((x)) { return VTSS_RC_ERROR;}

/* ================================================================= *
 *  Defects/Events
 * ================================================================= */

vtss_rc vtss_ewis_event_enable(const vtss_inst_t         inst,
                               const vtss_port_no_t      port_no,
                               const BOOL                enable,
                               const vtss_ewis_event_t   ev_mask)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK ) {
            rc = VTSS_FUNC_COLD(ewis_events_conf_set, port_no, enable, ev_mask);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_event_poll(const vtss_inst_t     inst,
                             const vtss_port_no_t  port_no,
                             vtss_ewis_event_t     *const status)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(status == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK ) {
            rc = VTSS_FUNC_COLD(ewis_events_poll, port_no, status);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}


#if defined(VTSS_ARCH_DAYTONA)
vtss_rc vtss_ewis_event_poll_without_mask(const vtss_inst_t     inst,
                                          const vtss_port_no_t  port_no,
                                          vtss_ewis_event_t     *const status)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(status == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK ) {
            rc = VTSS_FUNC_COLD(ewis_events_poll_without_mask, port_no, status);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_DAYTONA */

vtss_rc vtss_ewis_event_force(const vtss_inst_t        inst,
                              const vtss_port_no_t     port_no,
                              const BOOL               enable,
                              const vtss_ewis_event_t  ev_force)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (ev_force)) {
            rc = VTSS_FUNC_COLD(ewis_events_force, port_no, enable, ev_force);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_static_conf_get(const vtss_inst_t inst,
                                  const vtss_port_no_t port_no,
                                  vtss_ewis_static_conf_t *const stat_conf)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(stat_conf == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (stat_conf)) {
            *stat_conf = vtss_state->ewis_conf[port_no].static_conf;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_force_conf_set(const vtss_inst_t inst,
                                 const vtss_port_no_t port_no,
                                 const vtss_ewis_force_mode_t *const force_conf)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(force_conf == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (force_conf)) {
            vtss_state->ewis_conf[port_no].force_mode = *force_conf;
            rc = VTSS_FUNC_COLD(ewis_force_conf_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_force_conf_get(const vtss_inst_t inst,
                                 const vtss_port_no_t port_no,
                                 vtss_ewis_force_mode_t *const force_conf)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(force_conf == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (force_conf)) {
            *force_conf = vtss_state->ewis_conf[port_no].force_mode;
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_tx_oh_set(const vtss_inst_t inst,
                            const vtss_port_no_t port_no,
                            const vtss_ewis_tx_oh_t *const tx_oh)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(tx_oh == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (tx_oh)) {
            vtss_state->ewis_conf[port_no].tx_oh = *tx_oh;
            rc = VTSS_FUNC_COLD(ewis_tx_oh_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_tx_oh_get(const vtss_inst_t inst,
                            const vtss_port_no_t port_no,
                            vtss_ewis_tx_oh_t *const tx_oh)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(tx_oh == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (tx_oh)) {
            *tx_oh = vtss_state->ewis_conf[port_no].tx_oh;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_tx_oh_passthru_set(const vtss_inst_t inst,
                                     const vtss_port_no_t port_no,
                                     const vtss_ewis_tx_oh_passthru_t *const tx_oh_passthru)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(tx_oh_passthru == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (tx_oh_passthru)) {
            vtss_state->ewis_conf[port_no].tx_oh_passthru = *tx_oh_passthru;
            rc = VTSS_FUNC_COLD(ewis_tx_oh_passthru_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_tx_oh_passthru_get(const vtss_inst_t inst,
                                     const vtss_port_no_t port_no,
                                     vtss_ewis_tx_oh_passthru_t *const tx_oh_passthru)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(tx_oh_passthru == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (tx_oh_passthru)) {
            *tx_oh_passthru = vtss_state->ewis_conf[port_no].tx_oh_passthru;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_exp_sl_set (const vtss_inst_t inst,
                          const vtss_port_no_t port_no,
                          const vtss_ewis_sl_conf_t * const sl)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(sl == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            vtss_state->ewis_conf[port_no].exp_sl = *sl;
            rc = VTSS_FUNC_COLD(ewis_exp_sl_set, port_no);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

/* ================================================================= *
 *  Dynamic Config
 * ================================================================= */

vtss_rc vtss_ewis_mode_set(const vtss_inst_t inst,
                           const vtss_port_no_t port_no,
                           const vtss_ewis_mode_t *const mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(mode == NULL);
    VTSS_ENTER();
    do {
        if (((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) && (*mode < VTSS_WIS_OPERMODE_MAX)) {
            vtss_state->ewis_conf[port_no].ewis_mode = *mode;
            rc = VTSS_FUNC_COLD(ewis_mode_conf_set, port_no);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_mode_get(const vtss_inst_t inst,
                           const vtss_port_no_t port_no,
                           vtss_ewis_mode_t *const mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(mode == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            *mode = vtss_state->ewis_conf[port_no].ewis_mode;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_reset(const vtss_inst_t inst,
                       const vtss_port_no_t port_no)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK ) {
        rc = VTSS_FUNC_COLD(ewis_reset, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_cons_act_set(const vtss_inst_t inst,
                               const vtss_port_no_t port_no,
                               const vtss_ewis_cons_act_t *const cons_act)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(cons_act == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && cons_act) {
            vtss_state->ewis_conf[port_no].section_cons_act = *cons_act;
            rc = VTSS_FUNC_COLD(ewis_cons_action_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_cons_act_get(const vtss_inst_t inst,
                               const vtss_port_no_t port_no,
                               vtss_ewis_cons_act_t *const cons_act)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(cons_act == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && cons_act) {
            *cons_act = vtss_state->ewis_conf[port_no].section_cons_act;
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_section_txti_set(const vtss_inst_t inst,
                                   const vtss_port_no_t port_no,
                                   const vtss_ewis_tti_t *const txti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(txti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && txti->mode < TTI_MODE_MAX) {
            vtss_state->ewis_conf[port_no].section_txti = *txti;
            rc = VTSS_FUNC_COLD(ewis_section_txti_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_section_txti_get(const vtss_inst_t inst,
                                   const vtss_port_no_t port_no,
                                   vtss_ewis_tti_t *const txti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(txti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            *txti = vtss_state->ewis_conf[port_no].section_txti;
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_path_txti_set(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                const vtss_ewis_tti_t *const txti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(txti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && txti->mode < TTI_MODE_MAX) {
            vtss_state->ewis_conf[port_no].path_txti = *txti;
            rc = VTSS_FUNC_COLD(ewis_path_txti_set, port_no);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_ewis_path_txti_get(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                vtss_ewis_tti_t *const txti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(txti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            *txti = vtss_state->ewis_conf[port_no].path_txti;
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}


/* ================================================================= *
 *  Test Config/status
 * ================================================================= */

vtss_rc vtss_ewis_test_mode_set(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                const vtss_ewis_test_conf_t *const test_mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(test_mode == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && test_mode) {
            vtss_state->ewis_conf[port_no].test_conf = *test_mode;
            rc = VTSS_FUNC_COLD(ewis_test_mode_set, port_no);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_test_mode_get(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                vtss_ewis_test_conf_t *const test_mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(test_mode == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && test_mode) {
            *test_mode = vtss_state->ewis_conf[port_no].test_conf;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_prbs31_err_inj_set(const vtss_inst_t inst,
                                     const vtss_port_no_t port_no,
                                     const vtss_ewis_prbs31_err_inj_t *const inj)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(inj == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && inj) {
            rc = VTSS_FUNC_COLD(ewis_prbs31_err_inj_set, port_no, inj);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_ewis_test_counter_get(const vtss_inst_t inst,
                                   const vtss_port_no_t port_no,
                                   vtss_ewis_test_status_t *const test_status)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(test_status == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_test_status_get, port_no, test_status);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}


/* ================================================================= *
 *  State Reporting
 * ================================================================= */

vtss_rc vtss_ewis_defects_get(const vtss_inst_t inst,
                              const vtss_port_no_t port_no,
                              vtss_ewis_defects_t *const def)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(def == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_defects_get, port_no, def);
        }
    } while (0);
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_ewis_status_get(const vtss_inst_t inst,
                             const vtss_port_no_t port_no,
                             vtss_ewis_status_t *const status)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(status == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_status_get, port_no, status);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_section_acti_get(const vtss_inst_t inst,
                                   const vtss_port_no_t port_no,
                                   vtss_ewis_tti_t *const acti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(acti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_section_acti_get, port_no, acti);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_path_acti_get(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                vtss_ewis_tti_t *const acti)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(acti == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_path_acti_get, port_no, acti);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}


/* ================================================================= *
 *  Performance Primitives
 * ================================================================= */

vtss_rc vtss_ewis_perf_get(const vtss_inst_t inst,
                           const vtss_port_no_t port_no,
                           vtss_ewis_perf_t *const perf)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(perf == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_perf_get, port_no, perf);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_counter_get(const vtss_inst_t      inst,
                              const vtss_port_no_t   port_no,
                              vtss_ewis_counter_t *const counter)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(counter == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            rc = VTSS_FUNC_COLD(ewis_counter_get, port_no, counter);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_perf_mode_set(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                vtss_ewis_perf_mode_t const* perf_mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(perf_mode == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            vtss_state->ewis_conf[port_no].perf_mode = *perf_mode;
            rc = VTSS_FUNC_COLD(ewis_perf_mode_set, port_no);
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_perf_mode_get(const vtss_inst_t inst,
                                const vtss_port_no_t port_no,
                                vtss_ewis_perf_mode_t *const perf_mode)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(perf_mode == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            *perf_mode = vtss_state->ewis_conf[port_no].perf_mode;
        }
    } while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_counter_threshold_set(const vtss_inst_t inst,
                                        const vtss_port_no_t port_no,
                                        const vtss_ewis_counter_threshold_t *const threshold)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(threshold == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            vtss_state->ewis_conf[port_no].ewis_cntr_thresh_conf = *threshold;
            rc = VTSS_FUNC_COLD(ewis_counter_threshold_set, port_no);
        }
    }while(0);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ewis_counter_threshold_get(const vtss_inst_t inst,
                                        const vtss_port_no_t port_no,
                                        vtss_ewis_counter_threshold_t *const threshold)
{
    vtss_rc rc = VTSS_RC_ERROR;

    VTSS_D("port_no: %u", port_no);
    VTSS_PHY_EWIS_ASSERT(threshold == NULL);
    VTSS_ENTER();
    do {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
            *threshold = vtss_state->ewis_conf[port_no].ewis_cntr_thresh_conf;
        }
    }while(0);
    VTSS_EXIT();
    return rc;
}
#endif /*VTSS_FEATURE_WIS */
