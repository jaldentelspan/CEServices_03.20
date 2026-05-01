/*
 * vns_fpga_api.h
 *
 *  Created on: Aug 8
 *      Author: Jason Alden
 *      Copyright: Telspan Data 2014
 */

#ifndef GVCP_API_H_
#define GVCP_API_H_

#define TRACE_BUFF_LEN 1024
char trace_buff[TRACE_BUFF_LEN];


#include "main_types.h"     /* For vtss_init_data_t */
#include "vtss_module_id.h" /* For vtss_module_id_t */
//#include "vns_types.h"
//#include "vns_registers.h"
//#include "vtss_auth_api.h"
#include "vtss_api.h" /* For vtss_rc, vtss_vid_t, etc. */


#define GVCP_MGMT_MAX_HOSTKEY_LEN    (512)   /**< Maximum host key length */

/**
 * \brief gvcp configuration.
 */
typedef struct {
    unsigned int mode;                                   /* GVCP global mode */
    u8  rsa_hostkey[GVCP_MGMT_MAX_HOSTKEY_LEN];  /* Server certificate */
    unsigned int rsa_hostkey_len;                        /* Server certificate key length */
    u8  dss_hostkey[GVCP_MGMT_MAX_HOSTKEY_LEN];  /* Server private key */
    unsigned int dss_hostkey_len;                        /* Server private key length */
} gvcp_conf_t;

/**
 * \brief Module initialization function
 */
vtss_rc gvcp_init(vtss_init_data_t *data);

void StartGVCP(void);
/*
* Debug trace output
*/



#endif /* GVCP_API_H_ */
