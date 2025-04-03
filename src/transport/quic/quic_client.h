#ifndef quic_client__HEADER__INCLUDED
#define quic_client__HEADER__INCLUDED

#include "quic_record_marking.h"

#include "client_stream_context.h"
#include "streams.h"

typedef struct QuicClient {
    struct quic_endpoint_t *quic_endpoint;

    // underlying UDP socket
    int socket_fd;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    struct quic_config_t *quic_config;
    struct quic_tls_config_t *tls_config;
    struct quic_conn_t *quic_connection;

    struct ev_loop *event_loop;
    pthread_t event_loop_thread;
    bool successfully_created_event_loop_thread;
    ev_timer timer;

    pthread_mutex_t connection_established_lock;
    pthread_cond_t connection_established_condition_variable;
    bool connection_established;

    ev_async process_connections_async_watcher;

    bool use_auxilliary_stream;
    ev_async stream_allocator_async_watcher;
    Stream *allocated_stream;
    bool stream_allocation_finished;
    bool successfully_allocated_stream;
    pthread_mutex_t stream_allocator_lock;
    pthread_cond_t stream_allocator_condition_variable;

    ev_async connection_closing_async_watcher;
    pthread_mutex_t connection_closed_lock;
    pthread_cond_t connection_closed_condition_variable;
    bool connection_closed;

    ev_async event_loop_shutdown_async_watcher;

    Stream *main_stream;
    StreamsList *auxilliary_streams_list_head;
    pthread_mutex_t auxilliary_streams_list_lock;

    QuicClientStreamContextsList *stream_contexts;
    pthread_mutex_t stream_contexts_list_lock;
    pthread_cond_t stream_contexts_condition_variable;
} QuicClient;

#endif