/* -*- mode: C -*- */
[% INCLUDE copyright %]

#include "rpc_api.h"

/*
 * Utility functions
 */

void init_req(rpc_msg_t *msg, u32 id)
{
    msg->length = 0;
    msg->type = MSG_TYPE_REQ;
    msg->id = id;
}

void init_rsp(rpc_msg_t *msg)
{
    msg->type = MSG_TYPE_RSP;
    rpc_msg_init(msg, msg->data, msg->size);
}

void init_evt(rpc_msg_t *msg, u32 id)
{
    msg->length = 0;
    msg->type = MSG_TYPE_EVT;
    msg->id = id;
}

/*
 * Base de/encoders
 */
BOOL encode_u8(rpc_msg_t *msg, u8 arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_push(msg, 1))) {
        ptr[0] = arg;
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_push(msg, 4))) {
        ptr[0] = RPC_HTONL((u32) arg);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL encode_u16(rpc_msg_t *msg, u16 arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_push(msg, 2))) {
        ptr[0] = (u8)  arg;
        ptr[1] = (u8) (arg >> 8);
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_push(msg, 4))) {
        ptr[0] = RPC_HTONL((u32) arg);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL encode_u32(rpc_msg_t *msg, u32 arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_push(msg, 4))) {
        ptr[0] = (u8)  arg;
        ptr[1] = (u8) (arg >> 8);
        ptr[2] = (u8) (arg >> 16);
        ptr[3] = (u8) (arg >> 24);
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_push(msg, 4))) {
        ptr[0] = RPC_HTONL(arg);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL encode_u64(rpc_msg_t *msg, u64 arg)
{
    return 
        encode_u32(msg, (u32) arg) &&
        encode_u32(msg, (u32) (arg >> 32));
}

BOOL decode_u8(rpc_msg_t *msg, u8 *arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_pull(msg, 1))) {
        *arg = ptr[0];
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_pull(msg, 4))) {
        *arg = (u8) RPC_NTOHL(ptr[0]);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL decode_u16(rpc_msg_t *msg, u16 *arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_pull(msg, 2))) {
        *arg = (u16) ((ptr[0]) + 
                      (ptr[1] << 8));
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_pull(msg, 4))) {
        *arg = (u16) RPC_NTOHL(ptr[0]);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL decode_u32(rpc_msg_t *msg, u32 *arg)
{
#ifdef RPC_PACK
    u8 *ptr;
    if((ptr = rpc_msg_pull(msg, 4))) {
        *arg = (u32) ((ptr[0]) + 
                      (ptr[1] << 8) +
                      (ptr[2] << 16) +
                      (ptr[3] << 24));
        return TRUE;
    }
#else
    u32 *ptr;
    if((ptr = (u32*) rpc_msg_pull(msg, 4))) {
        *arg = RPC_NTOHL(ptr[0]);
        return TRUE;
    }
#endif
    return FALSE;
}

BOOL decode_u64(rpc_msg_t *msg, u64 *arg)
{
    u32 small, high;
    if(decode_u32(msg, &small) &&
       decode_u32(msg, &high)) {
        *arg = (((u64) high) << 32) + small;
        return TRUE;
    }
    return FALSE;
}

BOOL encode_rpc_pointer(rpc_msg_t *msg, rpc_pointer arg)
{
    return encode_u32(msg, (u32) arg);
}

BOOL decode_rpc_pointer(rpc_msg_t *msg, rpc_pointer *arg)
{
    u32 tmp;
    if(!decode_u32(msg, &tmp))
        return FALSE;
    *arg = (rpc_pointer) tmp;
    return TRUE;
}

/*
 * Application Type De/Encoders
 */
[% FOREACH t = types.type -%]
[% IF t.struct -%]
BOOL encode_[% t.name %](rpc_msg_t *msg, const [% t.name %] *arg)
{
    return [% "TRUE;" IF !t.elm.size %]
[% FOREACH e = t.elm -%]
[% IF get_type(e.base).struct -%]
        encode_[% e.base %](msg, &arg->[% e.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% ELSE -%]
        encode_[% e.base %](msg, arg->[% e.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
[% END -%]
}

BOOL decode_[% t.name %](rpc_msg_t *msg, [% t.name %] *arg)
{
    return [% "TRUE;" IF !t.elm.size %]
[% FOREACH e = t.elm -%]
        decode_[% e.base %](msg, &arg->[% e.name %])[% " &&" IF loop.next %][% ";" IF loop.last %]
[% END -%]
}

[% ELSE -%]
BOOL encode_[% t.name %](rpc_msg_t *msg, [% t.name %] arg)
{
    return encode_[% t.base %](msg, arg);
}

BOOL decode_[% t.name %](rpc_msg_t *msg, [% t.name %] *arg)
{
    [% t.base %] tmp;
    if(!decode_[% t.base %](msg, &tmp))
        return FALSE;
    *arg = ([% t.name %]) tmp;
    return TRUE;
}

[% END -%]
[% END -%]
