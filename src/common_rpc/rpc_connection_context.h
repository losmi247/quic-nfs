#ifndef rpc_connection_context__header__INCLUDED
#define rpc_connection_context__header__INCLUDED

#define _POSIX_C_SOURCE 200809L     // so we can use gethostname()

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/authentication/authentication.h"
#include "src/error_handling/error_handling.h"

#include "src/transport/transport_common.h"

/*
* 'Context' of a RPC connection specifies the IPv4 address and port of
* the server, the credential and verifier that should be sent as
* part of CallBody of any RPC call sent to this server,
* the identifier of the transport protocol to be used for sending RPCs,
* and the TCP client socket that is connected to the NFS server.
*/
typedef struct RpcConnectionContext {
    char *server_ipv4_addr;
    uint16_t server_port;

    Rpc__OpaqueAuth *credential;
    Rpc__OpaqueAuth *verifier;

    TransportProtocol transport_protocol;
    TransportConnection *transport_connection;
} RpcConnectionContext;

RpcConnectionContext *create_rpc_connection_context(char *server_ipv4_address, uint16_t server_port, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, TransportProtocol transport_protocol);

RpcConnectionContext *create_auth_none_rpc_connection_context(char *server_ipv4_address, uint16_t server_port, TransportProtocol transport_protocol);

RpcConnectionContext *create_auth_sys_rpc_connection_context(char *server_ipv4_address, uint16_t server_port, TransportProtocol transport_protocol);

void free_rpc_connection_context(RpcConnectionContext *rpc_connection_context);

#endif /* rpc_connection_context__header__INCLUDED */