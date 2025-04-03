#ifndef common_rpc__header__INCLUDED
#define common_rpc__header__INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#define NFS_RPC_MSG_BUFFER_SIZE 20000 // size of the buffer allocated for receiving a NFS RPC message

/*
 * Functions used in both client and server RPC program implementations.
 */

uint32_t generate_rpc_xid(void);

Rpc__RpcMsg *deserialize_rpc_msg(uint8_t *rpc_msg_buffer, size_t bytes_received);

void log_rpc_msg_info(Rpc__RpcMsg *rpc_msg);

void log_rpc_call_body_info(Rpc__CallBody *call_body);

#endif /* common_rpc__header__INCLUDED */