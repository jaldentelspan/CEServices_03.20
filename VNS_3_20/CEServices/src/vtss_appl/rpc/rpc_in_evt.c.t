/* -*- mode: C -*- */
[% INCLUDE copyright %]

#include "rpc_api.h"

/*
 * Event Decoding
 */

[% FOREACH evt = events.event -%]

static BOOL [% evt.name %]_decode(rpc_msg_t *msg, [% decllist(paramlist(evt, "in"),"ptr") %])
{
    return 
[% FOREACH param = paramlist(evt) -%]
        decode_[% param.type %](msg, [% param.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}
[% END -%]

void rpc_event_receive(rpc_msg_t *msg)
{
    switch(msg->id) {
[% FOREACH evt = events.event -%]
    case [% msgid(evt.name) %]: {
[% FOREACH param = paramlist(evt) -%]
        [% param.type %] [% param.name %];
[% END -%]
        if([% evt.name %]_decode(msg, [% namelist(paramlist(evt), "&") %])) {
            [% evt.name %]_receive([% namelist_p(paramlist(evt)) %]);
        } else {
            RPC_IOERROR("Decode of %s failed\n", "[% msgid(evt.name) %]");
        }
        break;
    }
[% END %]
        
    default:
        RPC_IOERROR("Invalid event: %d\n", msg->id);
    }
    rpc_message_dispose(msg);
}
