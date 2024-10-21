#ifndef client_common_rpc__header__INCLUDED
#define client_common_rpc__header__INCLUDED

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"

/*
* Functions implemented in client_common_rpc.c file.
*/

Rpc__RpcMsg *invoke_rpc(const char *server_ipv4_addr, uint16_t server_port, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters);

int validate_rpc_reply(Rpc__RpcMsg *rpc_reply);

Google__Protobuf__Any *extract_procedure_results_from_validated_rpc_reply(Rpc__RpcMsg *rpc_reply);

#endif /* client_common_rpc__header__INCLUDED */