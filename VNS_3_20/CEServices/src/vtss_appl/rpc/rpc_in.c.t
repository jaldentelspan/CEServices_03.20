/* -*- mode: C -*- */
[% INCLUDE copyright -%]

#include "rpc_api.h"

[% FOREACH group = groups %]
/*
 * Request Decoding: Group '[% group.name %]'
 */
[% FOREACH func = group.entry -%]
static BOOL [% func.name %]_decode_req(rpc_msg_t *msg, [% decllist(paramlist(func, "in"),"ptr") %])
{
    return 
[% FOREACH param = paramlist(func, "in") -%]
        decode_[% param.type %](msg, [% param.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}

[% END %]
[% END %]

[% FOREACH group = groups %]
/*
 * Response Encoding: Group '[% group.name %]'
 */
[% FOREACH func = group.entry -%]
[% IF nonempty(paramlist(func, "out")) -%]
static BOOL [% func.name %]_encode_rsp(rpc_msg_t *msg, [% decllist(paramlist(func, "out")) %])
{
    init_rsp(msg);
    return
[% FOREACH param = paramlist(func, "out") -%]
        encode_[% param.type %](msg, [% "*" IF !get_type(param.type).struct %][% param.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}

[% END -%]
[% END -%]
[% END -%]

void rpc_message_dispatch(rpc_msg_t *msg)
{
    vtss_rc rc = VTSS_RC_INCOMPLETE;
    RPC_IOTRACE("%s: message %d\n", __FUNCTION__, msg->id);
    switch(msg->id) {
[% FOREACH group = groups %]
[% FOREACH func = group.entry -%]
    case [% msgid(func.name) %]:
        {
[% FOREACH param = paramlist(func) -%]
            [% param.type %] [% param.name %];
[% END -%]
            RPC_IOTRACE("Processing %s\n", "[% msgid(func.name) %]");
            if(![% func.name %]_decode_req(msg, [% namelist(paramlist(func, "in"), "&") %])) {
                RPC_IOTRACE("Decode %s - failed\n", "[% msgid(func.name) %]");
            } else {
                rc = [% func.name %]([% namelist_p(paramlist(func)) %]);
                RPC_IOTRACE("Execute %s = %d\n", "[% msgid(func.name) %]", rc);
[% IF nonempty(paramlist(func, "out")) -%]
                if(![% func.name %]_encode_rsp(msg, [% namelist(paramlist(func, "out"), "&") %])) {
                    RPC_IOTRACE("Encode response %s - failed\n", "[% msgid(func.name) %]");
                    rc = VTSS_RC_INCOMPLETE;
                }
[% END -%]
            }
        }
        break;
[% END %]
[% END %]
    default:
        RPC_IOERROR("Illegal message type: %d\n", msg->id);
    }
    msg->rc = rc;
}
