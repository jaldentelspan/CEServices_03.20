/* -*- mode: C -*- */
[% INCLUDE copyright -%]

#include "rpc_api.h"

[% FOREACH group = groups %]
/*
 * Request Encoding: Group '[% group.name %]'
 */
[% FOREACH func = group.entry -%]
static BOOL [% func.name %]_encode_req(rpc_msg_t *msg, [% decllist(paramlist(func, "in")) %])
{
    init_req(msg, [% msgid(func.name) %]);
    return
[% FOREACH param = paramlist(func, "in") -%]
        encode_[% param.type %](msg, [% param.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}

[% END %]
[% END %]

[% FOREACH group = groups %]
/*
 * Request Decoding: Group '[% group.name %]'
 */
[% FOREACH func = group.entry -%]
[% IF nonempty(paramlist(func, "out")) -%]
static BOOL [% func.name %]_decode_rsp(rpc_msg_t *msg, [% decllist(paramlist(func, "out")) %])
{
    return
[% FOREACH param = paramlist(func, "out") -%]
        decode_[% param.type %](msg, [% param.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}
[% END -%]

[% END %]
[% END %]

[% FOREACH group = groups %]

/*
 * Shim API: Group '[% group.name %]'
 */
[% FOREACH func = group.entry -%]
vtss_rc rpc_[% func.name %]([% decllist(paramlist(func)) %])
{
    rpc_msg_t *msg = rpc_message_alloc();
    vtss_rc rc;
    if(!msg)
        return VTSS_RC_INCOMPLETE;
    if(![% func.name %]_encode_req(msg, [% namelist(paramlist(func, "in")) %]))
        return VTSS_RC_INCOMPLETE;
    rc = rpc_message_exec(msg);
[% IF nonempty(paramlist(func, "out")) -%]
    if(rc == VTSS_RC_OK) {
        // Copy out return values, rc, ...
        if(msg->rsp == NULL ||
           ![% func.name %]_decode_rsp(msg->rsp, [% namelist(paramlist(func, "out")) -%]))
            rc = VTSS_RC_INCOMPLETE;
    }
[% END -%]
    // Dispose message buffers
    if(msg->rsp != NULL)
        rpc_message_dispose(msg->rsp);
    rpc_message_dispose(msg);
    return rc;
}

[% END -%]
[% END -%]

