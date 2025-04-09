#include "quic_rpc_client.h"

#include "stream_allocation.h"
#include "streams.h"

void timeout_callback(EV_P_ ev_timer *w, int revents);
void process_connections(QuicClient *client);
void read_callback(EV_P_ ev_io *w, int revents);
void async_process_connections_callback(EV_P_ ev_async *w, int revents);
void async_stream_allocator_callback(EV_P_ ev_async *w, int revents);
void async_shutdown_loop_callback(EV_P_ ev_async *w, int revents);

// Callback handlers for QUIC events
void client_on_conn_created(void *tctx, struct quic_conn_t *conn) {
    QuicClient *client = tctx;
    client->quic_connection = conn;
}

/*
 * The function that the event loop thread runs.
 */
void *event_loop_runner(void *arg) {
    QuicClient *client = arg;

    client->event_loop = ev_loop_new(EVFLAG_AUTO);

    // initialize the process_connections callback
    ev_async_init(&client->process_connections_async_watcher, async_process_connections_callback);
    client->process_connections_async_watcher.data = client;
    ev_async_start(client->event_loop, &client->process_connections_async_watcher);

    // initialize the stream allocator callback
    ev_async_init(&client->stream_allocator_async_watcher, async_stream_allocator_callback);
    client->stream_allocator_async_watcher.data = client;
    ev_async_start(client->event_loop, &client->stream_allocator_async_watcher);

    // initialize the connection closing callback
    ev_async_init(&client->connection_closing_async_watcher, async_connection_closing_callback);
    client->connection_closing_async_watcher.data = client;
    ev_async_start(client->event_loop, &client->connection_closing_async_watcher);

    // initialize the event loop shutdown callback
    ev_async_init(&client->event_loop_shutdown_async_watcher, async_shutdown_loop_callback);
    client->event_loop_shutdown_async_watcher.data = client;
    ev_async_start(client->event_loop, &client->event_loop_shutdown_async_watcher);

    // initialize the timer
    ev_init(&client->timer, timeout_callback);
    client->timer.data = client;

    // process_connections(client);
    ev_async_send(client->event_loop, &client->process_connections_async_watcher);

    // start the event loop
    ev_io udp_socket_watcher;
    ev_io_init(&udp_socket_watcher, read_callback, client->socket_fd, EV_READ);
    udp_socket_watcher.data = client;
    ev_io_start(client->event_loop, &udp_socket_watcher);

    ev_run(client->event_loop, 0);

    return NULL;
}

/*
 * This function should be called from the RPC invoker thread to
 * kill the event loop thread if an error has happened.
 */
void kill_event_loop_thread(QuicClient *client) {
    if (client == NULL) {
        return;
    }

    ev_break(client->event_loop, EVBREAK_ALL);

    pthread_cancel(client->event_loop_thread);
    pthread_join(client->event_loop_thread, NULL);
}

void client_on_conn_established(void *tctx, struct quic_conn_t *conn) {
    QuicClient *client = tctx;

    Stream *main_stream = create_new_stream(client->quic_connection);
    if (main_stream == NULL) {
        fprintf(stderr, "client_on_conn_established: failed to allocate memory for the main stream\n");

        client->connection_established = true;
        pthread_cond_signal(&client->connection_established_condition_variable);

        return;
    }
    client->main_stream = main_stream;

    quic_stream_wantwrite(conn, main_stream->id, false);

    client->connection_established = true;
    pthread_cond_signal(&client->connection_established_condition_variable);
}

void client_on_conn_closed(void *tctx, struct quic_conn_t *conn) {
    QuicClient *client = tctx;

    client->connection_closed = true;

    ev_async_send(client->event_loop, &client->event_loop_shutdown_async_watcher);

    pthread_cond_signal(&client->connection_closed_condition_variable);
}

void client_on_stream_created(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
}

void client_on_stream_readable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    QuicClient *client = tctx;

    pthread_mutex_lock(&client->stream_contexts_list_lock);

    QuicClientStreamContext *stream_context = find_stream_context(client->stream_contexts, stream_id);
    if (stream_context == NULL) {
        fprintf(stderr,
                "client_on_stream_readable: client stream is readable but no response expected on this stream\n");
        pthread_mutex_unlock(&client->stream_contexts_list_lock);
        return;
    }

    if (stream_context->rm_receiving_context == NULL) {
        stream_context->rm_receiving_context = create_rm_receiving_context(conn, stream_id);

        if (stream_context->rm_receiving_context == NULL) {
            fprintf(stderr, "client_on_stream_readable: failed to create an RM receiving context\n");

            stream_context->reply_rpc_msg_successfully_received = false;

            stream_context->finished = true;
            kill_event_loop_thread(client);

            pthread_cond_signal(&stream_context->reply_cond);

            pthread_mutex_unlock(&client->stream_contexts_list_lock);

            return;
        }
    }

    int error_code = receive_available_bytes_quic(stream_context->rm_receiving_context);
    if (error_code > 0) {
        fprintf(stderr, "client_on_stream_readable: failed to read available data from the readable stream %ld\n",
                stream_id);

        stream_context->reply_rpc_msg_successfully_received = false;

        stream_context->finished = true;
        kill_event_loop_thread(client);

        pthread_cond_signal(&stream_context->reply_cond);

        pthread_mutex_unlock(&client->stream_contexts_list_lock);

        return;
    }

    // we've received a complete RPC on this stream
    if (stream_context->rm_receiving_context->record_fully_received) {
        stream_context->reply_rpc_msg_successfully_received = true;
        stream_context->finished = true;

        pthread_cond_signal(&stream_context->reply_cond);
    }

    pthread_mutex_unlock(&client->stream_contexts_list_lock);
}

void client_on_stream_writable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    QuicClient *client = tctx;

    pthread_mutex_lock(&client->stream_contexts_list_lock);

    QuicClientStreamContext *stream_context = find_stream_context(client->stream_contexts, stream_id);
    if (stream_context == NULL) {
        pthread_mutex_unlock(&client->stream_contexts_list_lock);
        return;
    }

    Stream *allocated_stream = stream_context->allocated_stream;
    if (allocated_stream == NULL || stream_id != allocated_stream->id) {
        fprintf(stderr, "client_on_stream_writable: a stream context was found for a non-designated stream\n");

        stream_context->finished = true;
        stream_context->call_rpc_msg_successfully_sent = false;
        kill_event_loop_thread(client);

        pthread_mutex_unlock(&client->stream_contexts_list_lock);

        return;
    }

    if (stream_context->attempted_call_rpc_msg_send) {
        pthread_mutex_unlock(&client->stream_contexts_list_lock);
        return;
    }

    // send the serialized RpcMsg to the server as a single Record Marking record on stream 0
    // TODO: (QNFS-37) implement time-outs + reconnections
    int error_code =
        send_rm_record_quic(conn, stream_id, stream_context->call_rpc_msg_buffer, stream_context->call_rpc_msg_size);
    stream_context->attempted_call_rpc_msg_send = true;
    stream_context->call_rpc_msg_successfully_sent = (error_code == 0);

    quic_stream_wantwrite(conn, stream_id, false);
    if (!stream_context->call_rpc_msg_successfully_sent) {
        kill_event_loop_thread(client);
    }

    pthread_mutex_unlock(&client->stream_contexts_list_lock);
}

void client_on_stream_closed(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
}

/*
 * Sends outbound QUIC packets via the underlying UDP socket.
 */
int client_on_packets_send(void *psctx, struct quic_packet_out_spec_t *pkts, unsigned int count) {
    QuicClient *client = psctx;

    unsigned int sent_count = 0;
    for (int i = 0; i < count; i++) {
        struct quic_packet_out_spec_t *pkt = pkts + i;
        for (int j = 0; j < (*pkt).iovlen; j++) {
            const struct iovec *iov = pkt->iov + j;
            ssize_t sent = sendto(client->socket_fd, iov->iov_base, iov->iov_len, 0, (struct sockaddr *)pkt->dst_addr,
                                  pkt->dst_addr_len);
            // we've sent a packet of 'sent' bytes

            if (sent != iov->iov_len) {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                    fprintf(stderr, "send would block, already sent: %d\n", sent_count);
                    return sent_count;
                }
                return -1;
            }
            sent_count++;
        }
    }

    return sent_count;
}

struct quic_transport_methods_t quic_transport_methods = {
    .on_conn_created = client_on_conn_created,
    .on_conn_established = client_on_conn_established,
    .on_conn_closed = client_on_conn_closed,
    .on_stream_created = client_on_stream_created,
    .on_stream_readable = client_on_stream_readable,
    .on_stream_writable = client_on_stream_writable,
    .on_stream_closed = client_on_stream_closed,
};

struct quic_packet_send_methods_t quic_packet_send_methods = {
    .on_packets_send = client_on_packets_send,
};

/*
 * Processes awaiting events on all QUIC connections at the given client's
 * endpoint, advancing the QUIC state machine.
 */
void process_connections(QuicClient *client) {
    quic_endpoint_process_connections(client->quic_endpoint);

    double timeout = quic_endpoint_timeout(client->quic_endpoint) / 1e3f;
    if (timeout < 0.0001) {
        timeout = 0.0001;
    }
    client->timer.repeat = timeout;
    ev_timer_again(client->event_loop, &client->timer);
}

/*
 * Forwards packets received (via the underlying UDP socket) to the client's QUIC endpoint.
 */
void read_callback(EV_P_ ev_io *w, int revents) {
    QuicClient *client = w->data;
    static uint8_t buf[READ_BUF_SIZE];

    while (true) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(client->socket_fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                // read would block, i.e. all data available now has been forwarded to QUIC
                break;
            }

            fprintf(stderr, "read_callback: failed to read\n");
            return;
        }

        quic_packet_info_t quic_packet_info = {
            .src = (struct sockaddr *)&peer_addr,
            .src_len = peer_addr_len,
            .dst = (struct sockaddr *)&client->local_addr,
            .dst_len = client->local_addr_len,
        };

        int error_code = quic_endpoint_recv(client->quic_endpoint, buf, read, &quic_packet_info);
        if (error_code != 0) {
            fprintf(stderr, "read_callback: recv failed with error %d\n", error_code);
            continue;
        }
    }

    ev_async_send(client->event_loop, &client->process_connections_async_watcher);
}

void timeout_callback(EV_P_ ev_timer *w, int revents) {
    QuicClient *client = w->data;
    quic_endpoint_on_timeout(client->quic_endpoint);
    ev_async_send(client->event_loop, &client->process_connections_async_watcher);
}

/*
 * Logs all QUIC events.
 *
 * To use, do 'quic_set_logger(debug_log, NULL, "TRACE");' when creating a QUIC endpoint.
 */
void debug_log(const uint8_t *data, size_t data_len, void *argp) {
    fwrite(data, sizeof(uint8_t), data_len, stderr);
}

void async_process_connections_callback(EV_P_ ev_async *w, int revents) {
    QuicClient *client = w->data;

    process_connections(client);
}

void async_connection_closing_callback(EV_P_ ev_async *w, int revents) {
    QuicClient *client = w->data;

    ev_async_send(client->event_loop, &client->process_connections_async_watcher);

    const char *reason = "ok";
    quic_conn_close(client->quic_connection, true, 0, (const uint8_t *)reason, strlen(reason));
}

void async_shutdown_loop_callback(EV_P_ ev_async *w, int revents) {
    QuicClient *client = w->data;

    ev_break(client->event_loop, EVBREAK_ALL);
}

/*
 * Progresses the event loop until the QUIC connection has been established.
 *
 * Returns 0 on success and > 0 on failure.
 */
int wait_for_connection_establishment(QuicClient *quic_client) {
    if (quic_client == NULL) {
        return 1;
    }

    if (quic_client->connection_established) {
        return 0;
    }

    // spawn the event loop thread
    if (pthread_create(&quic_client->event_loop_thread, NULL, event_loop_runner, quic_client) != 0) {
        fprintf(stderr, "wait_for_connection_establishment: failed to create the event loop thread\n");

        quic_client->successfully_created_event_loop_thread = false;

        return 2;
    }
    quic_client->successfully_created_event_loop_thread = true;

    pthread_mutex_lock(&quic_client->connection_established_lock);
    while (!quic_client->connection_established) {
        pthread_cond_wait(&quic_client->connection_established_condition_variable,
                          &quic_client->connection_established_lock);
    }
    pthread_mutex_unlock(&quic_client->connection_established_lock);

    if (quic_client->main_stream == NULL) {
        return 3;
    }

    return 0;
}

/*
 * Given a stream context, uses its condition variable to wait for the
 * RPC reply to to be received for the RPC call that is being or has been
 * sent on this stream.
 *
 * Returns 0 on success and > 0 on failure.
 */
int wait_for_rpc_reply(QuicClientStreamContext *stream_context) {
    if (stream_context == NULL) {
        return 1;
    }

    pthread_mutex_lock(&stream_context->reply_lock);
    while (!stream_context->finished) {
        pthread_cond_wait(&stream_context->reply_cond, &stream_context->reply_lock);
    }
    pthread_mutex_unlock(&stream_context->reply_lock);

    if (!stream_context->call_rpc_msg_successfully_sent) {
        fprintf(stderr, "wait_for_rpc_reply: failed to send RPC call message\n");
        return 2;
    }

    if (!stream_context->reply_rpc_msg_successfully_received) {
        fprintf(stderr, "wait_for_rpc_reply: failed to receive RPC reply message\n");
        return 3;
    }

    return 0;
}

/*
 * Sends an RPC call for the given program number, program version, procedure number, and parameters,
 * to the QUIC connection in the given RpcConnectionContext.
 *
 * If 'use_alternative_stream' is true, this function will attempt to send the RPC over one of the streams
 * not currently labeled as in use.
 *
 * Returns the RPC reply received from the server on success, and NULL on failure.
 *
 * The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)'
 * when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
 */
Rpc__RpcMsg *execute_rpc_call_quic(RpcConnectionContext *rpc_connection_context, Rpc__RpcMsg *call_rpc_msg,
                                   bool use_auxiliary_stream) {
    if (rpc_connection_context == NULL) {
        return NULL;
    }

    if (rpc_connection_context->transport_connection == NULL) {
        return NULL;
    }

    QuicClient *client = rpc_connection_context->transport_connection->quic_client;
    if (client == NULL) {
        return NULL;
    }

    if (call_rpc_msg == NULL) {
        return NULL;
    }

    int error_code = wait_for_connection_establishment(client);
    if (error_code != 0) {
        fprintf(stderr, "execute_rpc_call_quic: failed to wait for connection establishment\n");
        return NULL;
    }

    // serialize the RpcMsg to be sent
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(call_rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(sizeof(uint8_t) * rpc_msg_size);
    rpc__rpc_msg__pack(call_rpc_msg, rpc_msg_buffer);

    QuicClientStreamContext *stream_context =
        create_client_stream_context(client, rpc_msg_buffer, rpc_msg_size, use_auxiliary_stream);
    if (stream_context == NULL) {
        fprintf(stderr, "client_on_conn_established: failed to create a stream context\n");
        return NULL;
    }

    pthread_mutex_lock(&client->stream_allocation_queue_lock);
    error_code = push_to_stream_allocation_queue(stream_context, &client->stream_allocation_queue_back,
                                                 &client->stream_allocation_queue_front);
    pthread_mutex_unlock(&client->stream_allocation_queue_lock);

    // make the event loop thread execute the stream allocator callback, and wait for it to finish
    ev_async_send(client->event_loop, &client->stream_allocator_async_watcher);

    pthread_mutex_lock(&stream_context->stream_allocator_lock);
    while (!stream_context->stream_allocation_finished) {
        pthread_cond_wait(&stream_context->stream_allocator_condition_variable, &stream_context->stream_allocator_lock);
    }
    pthread_mutex_unlock(&stream_context->stream_allocator_lock);

    if (!stream_context->successfully_allocated_stream) {
        fprintf(stderr, "client_on_conn_established: failed to allocate a stream\n");

        free_stream_context(stream_context);

        return NULL;
    }

    pthread_mutex_lock(&client->stream_contexts_list_lock);
    error_code = add_stream_context(stream_context, &client->stream_contexts);
    pthread_mutex_unlock(&client->stream_contexts_list_lock);
    if (error_code != 0) {
        fprintf(stderr, "client_on_conn_established: failed to add stream context to collection of stream contexts\n");

        free_stream_context(stream_context);

        return NULL;
    }

    Rpc__RpcMsg *ret;

    // make the event loop thread execute the process connections callback
    ev_async_send(client->event_loop, &client->process_connections_async_watcher);

    error_code = wait_for_rpc_reply(stream_context);
    if (error_code != 0) {
        ret = NULL;
        goto cleanup;
    }

    Rpc__RpcMsg *reply_rpc_msg = deserialize_rpc_msg(stream_context->rm_receiving_context->accumulated_payloads,
                                                     stream_context->rm_receiving_context->accumulated_payloads_size);

    ret = reply_rpc_msg;

cleanup:
    Stream *allocated_stream = stream_context->allocated_stream;
    pthread_mutex_lock(&client->stream_contexts_list_lock);
    remove_stream_context(&client->stream_contexts, stream_context->allocated_stream->id);
    if (client->stream_contexts == NULL) {
        pthread_cond_signal(&client->stream_contexts_condition_variable);
    }
    pthread_mutex_unlock(&client->stream_contexts_list_lock);

    allocated_stream->stream_in_use = false;

    return ret;
}

/*
 * Given the RPC program number to be called, program version, procedure number, and the parameters for it, calls
 * the appropriate remote procedure over QUIC.
 *
 * Returns the server's RPC reply on success, and NULL on failure.
 *
 * The user of this function takes the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)'
 * when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
 */
Rpc__RpcMsg *invoke_rpc_remote_quic(RpcConnectionContext *rpc_connection_context, uint32_t program_number,
                                    uint32_t program_version, uint32_t procedure_number,
                                    Google__Protobuf__Any parameters, bool use_auxiliary_stream) {
    if (rpc_connection_context == NULL) {
        fprintf(stderr, "invoke_rpc_remote_quic: RpcConnectionContext is NULL\n");
        return NULL;
    }

    Rpc__CallBody call_body = RPC__CALL_BODY__INIT;
    call_body.rpcvers = 2;
    call_body.prog = program_number;
    call_body.vers = program_version;
    call_body.proc = procedure_number;

    call_body.credential = rpc_connection_context->credential;
    call_body.verifier = rpc_connection_context->verifier;

    call_body.params = &parameters;

    Rpc__RpcMsg call_rpc_msg = RPC__RPC_MSG__INIT;
    call_rpc_msg.xid = generate_rpc_xid();
    call_rpc_msg.mtype = RPC__MSG_TYPE__CALL;
    call_rpc_msg.body_case = RPC__RPC_MSG__BODY_CBODY; // this body_case enum is not actually sent over the network
    call_rpc_msg.cbody = &call_body;

    Rpc__RpcMsg *reply_rpc_msg = execute_rpc_call_quic(rpc_connection_context, &call_rpc_msg, use_auxiliary_stream);

    return reply_rpc_msg;
}