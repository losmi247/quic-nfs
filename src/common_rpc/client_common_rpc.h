#ifndef client_common_rpc__header__INCLUDED
#define client_common_rpc__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"

#include "record_marking.h"
#include "rpc_connection_context.h"

#include "src/error_handling/error_handling.h"

#include "src/parsing/parsing.h"

/*
* Functions implemented in client_common_rpc.c file.
*/

Rpc__RpcMsg *invoke_rpc_remote(RpcConnectionContext *rpc_connection_context, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters);

int validate_successful_accepted_reply(Rpc__RpcMsg *rpc_reply);

#endif /* client_common_rpc__header__INCLUDED */