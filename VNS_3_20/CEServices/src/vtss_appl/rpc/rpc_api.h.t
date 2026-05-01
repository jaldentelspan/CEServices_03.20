/* -*- mode: C -*- */
[% INCLUDE copyright -%]

/*
 * Include files
 */
[% FOREACH inc = includes.include -%]
#include "[% inc.file %]"
[% END -%]

typedef void *rpc_pointer;

/*
 * Enumerator
 */
enum {
[% FOREACH group = groups -%]
    /* Group  [% group.name %] */
[% FOREACH entry = group.entry -%]
    [% msgid(entry.name) %],
[% END -%]
[% END -%]
    /* Events */
[% FOREACH evt = events.event -%]
    [% msgid(evt.name) %],
[% END -%]
};

/***********************************************************************
 * Utility functions
 */

void init_req(rpc_msg_t *msg, u32 type);
void init_rsp(rpc_msg_t *msg);
void init_evt(rpc_msg_t *msg, u32 type);

/***********************************************************************
 * Base encoders
 */
BOOL encode_u8(rpc_msg_t *msg, u8 arg);
BOOL encode_u16(rpc_msg_t *msg, u16 arg);
BOOL encode_u32(rpc_msg_t *msg, u32 arg);
BOOL encode_rpc_pointer(rpc_msg_t *msg, rpc_pointer arg);
BOOL encode_u64(rpc_msg_t *msg, u64 arg);
BOOL decode_u8(rpc_msg_t *msg, u8 *arg);
BOOL decode_u16(rpc_msg_t *msg, u16 *arg);
BOOL decode_u32(rpc_msg_t *msg, u32 *arg);
BOOL decode_u64(rpc_msg_t *msg, u64 *arg);
BOOL decode_rpc_pointer(rpc_msg_t *msg, rpc_pointer *arg);

/***********************************************************************
 * Application Type Encoders
 */
[% FOREACH t = types.type -%]
BOOL decode_[% t.name %](rpc_msg_t *msg, [% t.name %] *arg);
[% IF t.struct -%]
BOOL encode_[% t.name %](rpc_msg_t *msg, const [% t.name %] *arg);
[% ELSE -%]
BOOL encode_[% t.name %](rpc_msg_t *msg, [% t.name %] arg);
[% END -%]
[% END -%]

/***********************************************************************
 * RPC 'wrapped' API
 */

[% FOREACH group = groups %]
/* Group '[% group.name %]' */
[% FOREACH func = group.entry -%]
vtss_rc rpc_[% func.name %]([% decllist(paramlist(func)) %]);
[% END %]
[% END %]

/***********************************************************************
 * Event generation and reception
 */

[% FOREACH evt = events.event -%]
/* 
 * Generate '[% evt.name %]' event (output side)
 */
vtss_rc rpc_[% evt.name %]([% decllist(paramlist(evt)) %]);

/* 
 * Consume '[% evt.name %]' event (input side - application defined)
 */
void [% evt.name %]_receive([% decllist(paramlist(evt)) %]);

[% END %]

void rpc_event_receive(rpc_msg_t *msg); /* Main event sink - RPC module provided */
