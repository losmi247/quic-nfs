#ifndef rpc_connection_context__header__INCLUDED
#define rpc_connection_context__header__INCLUDED

#define _POSIX_C_SOURCE 200809L     // so we can use gethostname()

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/authentication/authentication.h"

#include "src/error_handling/error_handling.h"

/*
* 'Context' of a RPC connection specifies the IPv4 address and port of
* the server, and the credential and verifier that should be sent as
* part of CallBody of any RPC call sent to this server.
*/
typedef struct RpcConnectionContext {
    char *server_ipv4_addr;
    uint16_t server_port;

    Rpc__OpaqueAuth *credential;
    Rpc__OpaqueAuth *verifier;
} RpcConnectionContext;

RpcConnectionContext *create_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier);

RpcConnectionContext *create_auth_none_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port);

RpcConnectionContext *create_auth_sys_rpc_connection_context(char *server_ipv4_addres, uint16_t server_port);

void free_rpc_connection_context(RpcConnectionContext *rpc_connection_context);

#endif /* rpc_connection_context__header__INCLUDED */