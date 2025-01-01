#ifndef tcp_client__header__INCLUDED
#define tcp_client__header__INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/rpc_connection_context.h"

#include "tcp_record_marking.h"

Rpc__RpcMsg *invoke_rpc_remote_tcp(RpcConnectionContext *rpc_connection_context, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters);

#endif /* tcp_client__header__INCLUDED */