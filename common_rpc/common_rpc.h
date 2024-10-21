#ifndef common_rpc__header__INCLUDED
#define common_rpc__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"

#define RPC_MSG_BUFFER_SIZE 500

/*
* Functions used in both client and server RPC program implementations.
*/

uint32_t generate_rpc_xid(void);

Rpc__RpcMsg *deserialize_rpc_msg(uint8_t *rpc_msg_buffer, size_t bytes_received);

#endif /* common_rpc__header__INCLUDED */