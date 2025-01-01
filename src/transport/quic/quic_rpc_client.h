#ifndef quic_client__header__INCLUDED
#define quic_client__header__INCLUDED

#define _OPEN_SYS_ITOA_EXT  // so we can use utoa()

#include <errno.h>
#include <ev.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "openssl/ssl.h"
#include "tquic.h"

#include "quic_record_marking.h"

#include "src/serialization/rpc/rpc.pb-c.h"

#include "src/common_rpc/common_rpc.h"
#include "src/common_rpc/rpc_connection_context.h"

#define MAX_DATAGRAM_SIZE 1200

struct QuicClient {
    struct quic_endpoint_t *quic_endpoint;

    // underlying UDP socket
    int socket_fd;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    struct quic_tls_config_t *tls_config;
    struct quic_conn_t *quic_connection;

    struct ev_loop *event_loop;
    ev_timer timer;

    // the RPC message to be sent by the client
    size_t call_rpc_msg_size;
    uint8_t *call_rpc_msg_buffer;

    // the RPC message being received as a Record Marking record
    RecordMarkingReceivingContext *rm_receiving_context;

    bool attempted_call_rpc_msg_send, call_rpc_msg_successfully_sent;
    bool reply_rpc_msg_successfully_received;
    bool finished;
};

/*
* RPC functions over QUIC
*/

Rpc__RpcMsg *invoke_rpc_remote_quic(RpcConnectionContext *rpc_connection_context, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters);

#endif /* quic_client__header__INCLUDED */