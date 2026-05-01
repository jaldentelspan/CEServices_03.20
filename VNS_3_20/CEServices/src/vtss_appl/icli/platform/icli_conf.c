/*

 Vitesse Switch Application software.

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

/*
******************************************************************************

    Revision history
    > CP.Wang, 2012/09/14 10:14
        - create

******************************************************************************
*/

/*
******************************************************************************

    Include File

******************************************************************************
*/
#include "conf_api.h"
#include "icli_api.h"
#include "icli_porting_trace.h"

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define ICLI_CONF_VERSION   0x0d001850

/*
******************************************************************************

    Type Definition

******************************************************************************
*/
typedef struct {
    u32                 version;
    icli_conf_data_t    conf;
} icli_conf_file_data_t;

/*
******************************************************************************

    Static Variable

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    save ICLI config data into file

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_conf_file_write(
    void
)
{
    icli_conf_file_data_t   *conf_file;
    ulong                   size;

    // get conf
    conf_file = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_ICLI, &size);
    if ( conf_file == NULL || size != sizeof(icli_conf_file_data_t)) {
        conf_file = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_ICLI, sizeof(icli_conf_file_data_t));
        if ( conf_file == NULL ) {
            T_E("fail to creat conf\n");
            return FALSE;
        }
    }

    // get ICLI config data
    if ( icli_conf_get( &(conf_file->conf), FALSE ) == FALSE ) {
        T_E("fail to get ICLI config\n");
        return FALSE;
    }

    // get version
    conf_file->version = ICLI_CONF_VERSION;

    // write to file
    conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_ICLI);

    // successful
    return TRUE;
}

/*
    load and apply ICLI config data from file

    INPUT
        b_default : create default config or not

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_conf_file_load(
    IN BOOL     b_default
)
{
    icli_conf_file_data_t   *conf_file;
    ulong                   size;

    // get conf
    conf_file = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_ICLI, &size);
    if ( conf_file == NULL || size != sizeof(icli_conf_file_data_t)) {
        conf_file = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_ICLI, sizeof(icli_conf_file_data_t));
        if ( conf_file == NULL ) {
            T_E("fail to creat conf\n");
            return FALSE;
        }
    }

    // version check
    if ( conf_file->version != ICLI_CONF_VERSION ) {
        b_default = TRUE;
    }

    // reset to default
    if ( b_default ) {
        if ( icli_conf_default( &(conf_file->conf) ) == FALSE ) {
            T_E("fail to reset conf to default\n");
            return FALSE;
        }
    }

    // apply conf
    if ( icli_conf_apply( &(conf_file->conf) ) == FALSE ) {
        T_E("fail to apply conf\n");
        return FALSE;
    }

    // get version
    conf_file->version = ICLI_CONF_VERSION;

    // close conf
    conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_ICLI);

    // successful
    return TRUE;
}
