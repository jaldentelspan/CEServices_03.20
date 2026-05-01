#ifdef VTSS_SW_OPTION_ICFG
#include "ip2_icli_priv.h"
#include "ip2_api.h"
#include "ip2_utils.h"
#include "icli_porting.h"
#include "icli_types.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif

#define PRINTF(...) (void)vtss_icfg_printf(result, __VA_ARGS__);

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP2

#ifdef VTSS_SW_OPTION_ICFG
static vtss_rc IP2_ipv4_icfg_conf(const vtss_icfg_query_request_t *req,
                                  vtss_icfg_query_result_t *result)
{
    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG: {
        vtss_rc rc;
        int cnt, i;
        vtss_ip2_global_param_t conf;
        vtss_routing_entry_t *routes = VTSS_CALLOC(IP2_MAX_ROUTES, sizeof(*routes));

        if (!routes) {
            return VTSS_RC_ERROR;
        }

        rc = vtss_ip2_global_param_get(&conf);
        if (rc != VTSS_OK) {
            goto ROUTE_DONE;
        }

        if (conf.enable_routing) {
            PRINTF("ip routing\n");
        }

        /*lint --e{429} */
        rc = vtss_ip2_route_conf_get(IP2_MAX_ROUTES, routes, &cnt);
        if (rc != VTSS_OK) {
            goto ROUTE_DONE;
        }

        for (i = 0; i < cnt; i++) {
            vtss_ipv4_t mask;
            vtss_routing_entry_t const *rt = &routes[i];

            (void)
            vtss_conv_prefix_to_ipv4mask(rt->route.ipv4_uc.network.prefix_size,
                                         &mask);
            if (rt->type == VTSS_ROUTING_ENTRY_TYPE_IPV4_UC) {
                PRINTF("ip route "VTSS_IPV4_FORMAT" "VTSS_IPV4_FORMAT
                       " "VTSS_IPV4_FORMAT"\n",
                       VTSS_IPV4_ARGS(rt->route.ipv4_uc.network.address),
                       VTSS_IPV4_ARGS(mask),
                       VTSS_IPV4_ARGS(rt->route.ipv4_uc.destination));
            }
        }

ROUTE_DONE:
        VTSS_FREE(routes);
        if (rc != VTSS_RC_OK) {
            return rc;
        }
        break;
    }

    case ICLI_CMD_MODE_INTERFACE_VLAN: {
        vtss_vid_t vid = (vtss_vid_t)req->instance_id.vlan;
        vtss_ip_conf_t ip_conf;

        if (vtss_ip2_ipv4_conf_get(vid, &ip_conf) == VTSS_RC_OK) {
            vtss_ipv4_t mask;
            (void) vtss_conv_prefix_to_ipv4mask(ip_conf.network.prefix_size,
                                                &mask);
            if (ip_conf.dhcpc) {
                PRINTF(" ip address dhcp\n");
            } else if (ip_conf.network.address.type == VTSS_IP_TYPE_IPV4) {
                PRINTF(" ip address "VTSS_IPV4_FORMAT" "VTSS_IPV4_FORMAT"\n",
                       VTSS_IPV4_ARGS(ip_conf.network.address.addr.ipv4),
                       VTSS_IPV4_ARGS(mask));
            } else {
                PRINTF(" no ip address\n");
            }
        } else {
            PRINTF(" no ip address\n");
        }

        break;
    }

    default:
        //Not needed
        break;
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_ICFG */

vtss_rc vtss_ip2_ipv4_icfg_init(void)
{
#ifdef VTSS_SW_OPTION_ICFG
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_IPV4_GLOBAL, "ipv4",
                                     IP2_ipv4_icfg_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_IPV4_INTERFACE, "ipv4",
                                     IP2_ipv4_icfg_conf));
#endif
    return VTSS_RC_OK;
}

#endif // VTSS_SW_OPTION_ICFG

