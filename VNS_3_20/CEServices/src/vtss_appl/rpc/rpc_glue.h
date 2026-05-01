/*

 Vitesse Switch API software.

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

#ifndef _VTSS_IPC_GLUE_H_
#define _VTSS_RPC_GLUE_H_

#include <network.h>            /* htonl/ntohl */
#include "main.h"
#include "dualcpu.h"

enum {
    MSG_TYPE_REQ,               /* Request */
    MSG_TYPE_RSP,               /* Response */
    MSG_TYPE_EVT,               /* Event (unsolicited indication) */
};

typedef struct rpc_msg {
    /* Local householding */
    struct rpc_msg *next;     /* Link pointer, for request queueing use */
    struct rpc_msg *rsp;      /* Link pointer, for response message */
    void *syncro;             /* Synchronization object pointer */
    u32 timeout;              /* For implementing message timeouts */
    /* Communication Message Fields */
    u16		   id;          /* Request or event ordinal */
    u16		   type;        /* Message type */
    u32		   rc;          /* Return code from request execution */
    u32		   seq;         /* Message sequence number for correlation */
    /* Buffer management */
    size_t         size;        /* Total (max) length of buffer */
    size_t         available;   /* Current bytes available in buffer */
    size_t         length;      /* Current bytes in buffer */
    u8             *data;       /* Start of buffer */
    u8             *head;       /* Current start of buffer */
    u8             *tail;       /* Current end of buffer */
} rpc_msg_t;

static inline void rpc_msg_init(rpc_msg_t *msg, u8 *ptr, size_t size)
{
    msg->size = msg->available = size;
    msg->length = 0;
    msg->data = msg->head = msg->tail = ptr;
}

static inline u8 *rpc_msg_push(rpc_msg_t *msg, size_t adj)
{
    if(adj <= msg->available) {
        u8 *ptr = msg->tail;
        msg->available -= adj;
        msg->length += adj;
        msg->tail += adj;
        return ptr;
    }
    return NULL;
}

static inline u8 *rpc_msg_pull(rpc_msg_t *msg, size_t adj)
{
    if(adj <= msg->length) {
        u8 *ptr = msg->head;
        msg->length -= adj;
        msg->head += adj;
        return ptr;
    }
    return NULL;
}

/*
 * Timout (in seconds). If a request has not received a response
 * within this period, it will time out.
 */
#define RPC_TIMEOUT 3

/*
 * Marshalling options
 *
 * RPC_PACK: Set if data objects are byte-packed in a consistent,
 * CPU-independent manner.
 *
 * Otherwise, data objects are 32-bit aligned and the byteorder
 * defined by the RPC_NTOHL(), RPC_HTONL, macros.
 */
//#define RPC_PACK 1
#ifdef RPC_NETWORK_CODING
#define RPC_NTOHL(x) ntohl(x)
#define RPC_HTONL(x) htonl(x)
#else
#define RPC_NTOHL(x) (x)
#define RPC_HTONL(x) (x)
#endif

/*
 * Trace and error reporting
 */
#define RPC_IOERROR(...) printf(__VA_ARGS__)
#if 0
#define RPC_IOTRACE(...) printf(__VA_ARGS__)
#else
#define RPC_IOTRACE(...)
#endif

void       rpc_message_dispatch(rpc_msg_t *msg);
rpc_msg_t *rpc_message_alloc(void);
void       rpc_message_dispose(rpc_msg_t *msg);
vtss_rc    rpc_message_exec(rpc_msg_t *msg);
vtss_rc    rpc_message_send(rpc_msg_t *msg);

#endif /* _VTSS_RPC_GLUE_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
