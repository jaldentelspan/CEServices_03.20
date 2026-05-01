/*


*/

#ifndef _GVCP_H_
#define _GVCP_H_

#include "gvcp_api.h"
#include "vtss_module_id.h"
#include "critd_api.h"

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>


#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_GVCP
#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_CNT          1
/* ============== */
#include <vtss_trace_api.h>
#include  "packet_api.h"



/* GVCP configuration block */
typedef struct {
    unsigned int         version;    /* Block version */
    gvcp_conf_t  gvcp_conf;   /* gvcp configuration */
} gvcp_conf_blk_t;


typedef struct {
    critd_t             crit;
    gvcp_conf_t   gvcp_conf;
} gvcp_global_t;


#endif /* _GVCP_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
