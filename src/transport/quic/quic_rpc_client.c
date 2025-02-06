#include "quic_rpc_client.h"

// Callback handlers for QUIC events
void client_on_conn_created(void *tctx, struct quic_conn_t *conn) {
    QuicClient *client = tctx;
    client->quic_client_connection_context->quic_connection = conn;
}

void client_on_conn_established(void *tctx, struct quic_conn_t *conn) {
    QuicClient *client = tctx;

    uint64_t new_stream_id;
    int error_code = quic_stream_bidi_new(conn, 0, true, &new_stream_id);
    client->quic_client_connection_context->attempted_to_create_main_stream = true;
    if(error_code != 0) {
        fprintf(stderr, "client_on_conn_established: failed to create the main stream\n");

        client->quic_client_connection_context->main_stream_created_successfully = false;

        client->finished = true;
        ev_break(client->event_loop, EVBREAK_ALL);

        return;
    }

    client->quic_client_connection_context->main_stream_created_successfully = true;
    client->quic_client_connection_context->main_stream_id = new_stream_id;

    quic_stream_wantwrite(conn, new_stream_id, true);
}

void client_on_conn_closed(void *tctx, struct quic_conn_t *conn) {
    printf("WTF WHY IS THE CONNECTION CLOSING\n"); fflush(stdout);
}

void client_on_stream_created(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

void client_on_stream_readable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    QuicClient *client = tctx;

    if(client->rm_receiving_context == NULL) {
        client->rm_receiving_context = create_rm_receiving_context(conn, stream_id);

        if(client->rm_receiving_context == NULL) {
            fprintf(stderr, "client_on_stream_readable: failed to create a RM receiving context\n");
            return;
        }
    }

    int error_code = receive_available_bytes_quic(client->rm_receiving_context);
    if(error_code > 0) {
        fprintf(stderr, "client_on_stream_readable: failed to read available data from the readable stream %ld\n", stream_id);

        client->reply_rpc_msg_successfully_received = false;

        client->finished = true;
        ev_break(client->event_loop, EVBREAK_ALL);

        return;
    }

    // we've received a complete RPC on this stream
    if(client->rm_receiving_context->record_fully_received) {
        client->reply_rpc_msg_successfully_received = true;

        client->finished = true;
        ev_break(client->event_loop, EVBREAK_ALL);
    }
}

void client_on_stream_writable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    quic_stream_wantwrite(conn, stream_id, false);

    QuicClient *client = tctx;
    
    if(client->attempted_call_rpc_msg_send) {
        fprintf(stderr, "client_on_stream_writable: QUIC client attempted to resend the RPC\n");

        client->resend_attempted = true;
        ev_break(client->event_loop, EVBREAK_ALL);

        return;
    }

    // send the serialized RpcMsg to the server as a single Record Marking record on stream 0
    // TODO: (QNFS-37) implement time-outs + reconnections
    int error_code = send_rm_record_quic(conn, stream_id, client->call_rpc_msg_buffer, client->call_rpc_msg_size);
    client->attempted_call_rpc_msg_send = true;
    client->call_rpc_msg_successfully_sent = (error_code == 0);

    if(!client->call_rpc_msg_successfully_sent) {
        ev_break(client->event_loop, EVBREAK_ALL);
    }
}

void client_on_stream_closed(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

/*
* Sends outbound QUIC packets via the underlying UDP socket.
*/
int client_on_packets_send(void *psctx, struct quic_packet_out_spec_t *pkts, unsigned int count) {
    QuicClient *client = psctx;

    unsigned int sent_count = 0;
    for(int i = 0; i < count; i++) {
        struct quic_packet_out_spec_t *pkt = pkts + i;
        for(int j = 0; j < (*pkt).iovlen; j++) {
            const struct iovec *iov = pkt->iov + j;
            ssize_t sent = sendto(client->quic_client_connection_context->socket_fd, iov->iov_base, iov->iov_len, 0, (struct sockaddr *)pkt->dst_addr, pkt->dst_addr_len);
            // we've sent a packet of 'sent' bytes

            if(sent != iov->iov_len) {
                if((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
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
* Processes awaiting events on all QUIC connections at the given server's
* endpoint, advancing the QUIC state machine.
*/
static void process_connections(QuicClient *client) {
    quic_endpoint_process_connections(client->quic_client_connection_context->quic_endpoint);

    double timeout = quic_endpoint_timeout(client->quic_client_connection_context->quic_endpoint) / 1e3f;
    if(timeout < 0.0001) {
        timeout = 0.0001;
    }
    client->timer.repeat = timeout;
    ev_timer_again(client->event_loop, &client->timer);
}

/*
* Forwards packets received (via the underlying UDP socket) to the client's QUIC endpoint.
*/
static void read_callback(EV_P_ ev_io *w, int revents) {
    QuicClient *client = w->data;
    static uint8_t buf[READ_BUF_SIZE];

    while(true) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(client->quic_client_connection_context->socket_fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if(read < 0) {
            if((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                // read would block, i.e. all data available now has been forwarded to QUIC
                break;
            }

            fprintf(stderr, "read_callback: failed to read\n");
            return;
        }

        quic_packet_info_t quic_packet_info = {
            .src = (struct sockaddr *)&peer_addr,
            .src_len = peer_addr_len,
            .dst = (struct sockaddr *)&client->quic_client_connection_context->local_addr,
            .dst_len = client->quic_client_connection_context->local_addr_len,
        };

        int error_code = quic_endpoint_recv(client->quic_client_connection_context->quic_endpoint, buf, read, &quic_packet_info);
        if(error_code != 0) {
            fprintf(stderr, "read_callback: recv failed with error %d\n", error_code);
            continue;
        }
    }

    process_connections(client);
}

static void timeout_callback(EV_P_ ev_timer *w, int revents) {
    QuicClient *client = w->data;
    quic_endpoint_on_timeout(client->quic_client_connection_context->quic_endpoint);
    process_connections(client);
}

/*
* Logs all QUIC events.
*
* To use, do 'quic_set_logger(debug_log, NULL, "TRACE");' when creating a QUIC endpoint.
*/
static void debug_log(const uint8_t *data, size_t data_len, void *argp) {
    fwrite(data, sizeof(uint8_t), data_len, stderr);
}

/*
* Sends an RPC call for the given program number, program version, procedure number, and parameters,
* to the QUIC connection in the given RpcConnectionContext.
*
* Returns the RPC reply received from the server on success, and NULL on failure.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *execute_rpc_call_quic(RpcConnectionContext *rpc_connection_context, Rpc__RpcMsg *call_rpc_msg) {
    if(rpc_connection_context == NULL) {
        return NULL;
    }

    if(rpc_connection_context->transport_connection == NULL) {
        return NULL;
    }

    QuicClient *client = rpc_connection_context->transport_connection->quic_client;
    if(client == NULL) {
        return NULL;
    }

    if(call_rpc_msg == NULL) {
        return NULL;
    }

    client->event_loop = NULL;

    client->rm_receiving_context = NULL;

    client->resend_attempted = false;
    client->attempted_call_rpc_msg_send = client->call_rpc_msg_successfully_sent = false;
    client->reply_rpc_msg_successfully_received = false;
    client->finished = false;

    Rpc__RpcMsg *ret;

    // serialize the RpcMsg to be sent
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(call_rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(sizeof(uint8_t) * rpc_msg_size);
    rpc__rpc_msg__pack(call_rpc_msg, rpc_msg_buffer);
    client->call_rpc_msg_size = rpc_msg_size;
    client->call_rpc_msg_buffer = rpc_msg_buffer;

    // if the main stream has been created, clients wants to write to it
    if(client->quic_client_connection_context->main_stream_created_successfully) {
        quic_stream_wantwrite(client->quic_client_connection_context->quic_connection, client->quic_client_connection_context->main_stream_id, true);
    }

    // initialize the event loop
    client->event_loop = ev_default_loop(0);
    ev_init(&client->timer, timeout_callback);
    client->timer.data = client;

    process_connections(client);

    // start the event loop
    ev_io udp_socket_watcher;
    ev_io_init(&udp_socket_watcher, read_callback, client->quic_client_connection_context->socket_fd, EV_READ);
    udp_socket_watcher.data = client;
    ev_io_start(client->event_loop, &udp_socket_watcher);

    ev_run(client->event_loop, 0);

    if(!client->quic_client_connection_context->main_stream_created_successfully) {
        fprintf(stderr, "execute_rpc_call_quic: failed to create the main stream\n");
        ret = NULL;
        goto cleanup;
    }

    if(client->resend_attempted) {
        fprintf(stderr, "execute_rpc_call_quic: QUIC client attempted to resend an RPC\n");
        ret = NULL;
        goto cleanup;
    }

    if(!client->call_rpc_msg_successfully_sent) {
        fprintf(stderr, "execute_rpc_call_quic: failed to send RPC call message\n");
        ret = NULL;
        goto cleanup;
    }

    if(!client->reply_rpc_msg_successfully_received) {
        fprintf(stderr, "execute_rpc_call_quic: failed to receive RPC reply message\n");
        ret = NULL;
        goto cleanup;
    }

    Rpc__RpcMsg *reply_rpc_msg = deserialize_rpc_msg(client->rm_receiving_context->accumulated_payloads, client->rm_receiving_context->accumulated_payloads_size);

    ret = reply_rpc_msg;

cleanup:
    if(client->call_rpc_msg_buffer != NULL) {
        free(client->call_rpc_msg_buffer);
    }
    if(client->event_loop != NULL) {
        ev_loop_destroy(client->event_loop);
    }
    free_rm_receiving_context(client->rm_receiving_context);

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
Rpc__RpcMsg *invoke_rpc_remote_quic(RpcConnectionContext *rpc_connection_context, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
    if(rpc_connection_context == NULL) {
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

    Rpc__RpcMsg *reply_rpc_msg = execute_rpc_call_quic(rpc_connection_context, &call_rpc_msg);

    return reply_rpc_msg;
}