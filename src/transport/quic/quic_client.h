#ifndef quic_client__HEADER__INCLUDED
#define quic_client__HEADER__INCLUDED

#include "quic_record_marking.h"

typedef struct QuicClientConnectionContext {
    struct quic_endpoint_t *quic_endpoint;

    // underlying UDP socket
    int socket_fd;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    struct quic_config_t *quic_config;
    struct quic_tls_config_t *tls_config;
    struct quic_conn_t *quic_connection;

    uint64_t main_stream_id;
    bool attempted_to_create_main_stream;
    bool main_stream_created_successfully;
} QuicClientConnectionContext;

typedef struct QuicClient {
    QuicClientConnectionContext *quic_client_connection_context;

    struct ev_loop *event_loop;
    ev_timer timer;

    // the RPC message to be sent by the client
    size_t call_rpc_msg_size;
    uint8_t *call_rpc_msg_buffer;

    // the RPC message being received as a Record Marking record
    RecordMarkingReceivingContext *rm_receiving_context;

    bool resend_attempted;
    bool attempted_call_rpc_msg_send, call_rpc_msg_successfully_sent;
    bool reply_rpc_msg_successfully_received;
    bool finished;
} QuicClient;

#endif