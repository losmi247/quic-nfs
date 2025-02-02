#include "quic_rpc_client.h"

// Callback handlers for QUIC events
void client_on_conn_created(void *tctx, struct quic_conn_t *conn) {
    struct QuicClient *client = tctx;
    client->quic_connection = conn;
}

void client_on_conn_established(void *tctx, struct quic_conn_t *conn) {
    struct QuicClient *client = tctx;

    // send the serialized RpcMsg to the server as a single Record Marking record on stream 0
    // TODO: (QNFS-37) implement time-outs + reconnections
    int error_code = send_rm_record_quic(conn, 0, client->call_rpc_msg_buffer, client->call_rpc_msg_size);
    client->call_rpc_msg_successfully_sent = (error_code == 0);

    if(!client->call_rpc_msg_successfully_sent) {
        ev_break(client->event_loop, EVBREAK_ALL);
    }
}

void client_on_conn_closed(void *tctx, struct quic_conn_t *conn) {
    struct QuicClient *client = tctx;

    client->finished = true;
    ev_break(client->event_loop, EVBREAK_ALL);
}

void client_on_stream_created(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

void client_on_stream_readable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    struct QuicClient *client = tctx;

    if(client->rm_receiving_context == NULL) {
        client->rm_receiving_context = create_rm_receiving_context(conn ,stream_id);

        if(client->rm_receiving_context == NULL) {
            fprintf(stderr, "client_on_stream_readable: failed to create a RM receiving context\n");
            return;
        }
    }

    int error_code = receive_available_bytes_quic(client->rm_receiving_context);
    if(error_code > 0) {
        fprintf(stderr, "client_on_stream_readable: failed to read available data from the readable stream %ld\n", stream_id);

        client->reply_rpc_msg_successfully_received = false;

        const char *reason = "failed to read available data";
        quic_conn_close(conn, true, 0, (const uint8_t *)reason, strlen(reason));

        return;
    }

    // we've received a complete RPC on this stream
    if(client->rm_receiving_context->record_fully_received) {
        client->reply_rpc_msg_successfully_received = true;

        const char *reason = "ok";
        quic_conn_close(conn, true, 0, (const uint8_t *)reason, strlen(reason));
    }
}

void client_on_stream_writable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    quic_stream_wantwrite(conn, stream_id, false);
}

void client_on_stream_closed(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

/*
* Sends outbound QUIC packets via the underlying UDP socket.
*/
int client_on_packets_send(void *psctx, struct quic_packet_out_spec_t *pkts, unsigned int count) {
    struct QuicClient *client = psctx;

    unsigned int sent_count = 0;
    for(int i = 0; i < count; i++) {
        struct quic_packet_out_spec_t *pkt = pkts + i;
        for(int j = 0; j < (*pkt).iovlen; j++) {
            const struct iovec *iov = pkt->iov + j;
            ssize_t sent = sendto(client->socket_fd, iov->iov_base, iov->iov_len, 0, (struct sockaddr *)pkt->dst_addr, pkt->dst_addr_len);
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

const struct quic_transport_methods_t quic_transport_methods = {
    .on_conn_created = client_on_conn_created,
    .on_conn_established = client_on_conn_established,
    .on_conn_closed = client_on_conn_closed,
    .on_stream_created = client_on_stream_created,
    .on_stream_readable = client_on_stream_readable,
    .on_stream_writable = client_on_stream_writable,
    .on_stream_closed = client_on_stream_closed,
};

const struct quic_packet_send_methods_t quic_packet_send_methods = {
    .on_packets_send = client_on_packets_send,
};

/*
* Processes awaiting events on all QUIC connections at the given server's
* endpoint, advancing the QUIC state machine.
*/
static void process_connections(struct QuicClient *client) {
    quic_endpoint_process_connections(client->quic_endpoint);

    double timeout = quic_endpoint_timeout(client->quic_endpoint) / 1e3f;
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
    struct QuicClient *client = w->data;
    static uint8_t buf[READ_BUF_SIZE];

    while(true) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(client->socket_fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
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
            .dst = (struct sockaddr *)&client->local_addr,
            .dst_len = client->local_addr_len,
        };

        int error_code = quic_endpoint_recv(client->quic_endpoint, buf, read, &quic_packet_info);
        if(error_code != 0) {
            fprintf(stderr, "read_callback: recv failed with error %d\n", error_code);
            continue;
        }
    }

    process_connections(client);
}

static void timeout_callback(EV_P_ ev_timer *w, int revents) {
    struct QuicClient *client = w->data;
    quic_endpoint_on_timeout(client->quic_endpoint);
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
* Creates an underlying UDP socket which can be used for sending QUIC packets. 
*/
static int create_socket(const char *host, const char *port, struct addrinfo **peer, struct QuicClient *client) {
    const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
    if(getaddrinfo(host, port, &hints, peer) != 0) {
        fprintf(stderr, "create_socket: failed to resolve host\n");
        return 1;
    }

    int udp_socket = socket((*peer)->ai_family, SOCK_DGRAM, 0);
    if(udp_socket < 0) {
        fprintf(stderr, "create_socket: failed to create socket\n");
        return 1;
    }
    if(fcntl(udp_socket, F_SETFL, O_NONBLOCK) != 0) {
        fprintf(stderr, "create_socket: failed to make socket non-blocking\n");
        return 1;
    }

    client->local_addr_len = sizeof(client->local_addr);
    if(getsockname(udp_socket, (struct sockaddr *)&client->local_addr, &client->local_addr_len) != 0) {
        fprintf(stderr, "create_socket: failed to get local address of socket\n");
        return 1;
    };
    client->socket_fd = udp_socket;

    return 0;
}

Rpc__RpcMsg *execute_rpc_call_quic(const char *host, const char *port, Rpc__RpcMsg *call_rpc_msg) {
    if(call_rpc_msg == NULL) {
        return NULL;
    }

    struct QuicClient client;
    client.quic_endpoint = NULL;
    client.tls_config = NULL;
    client.quic_connection = NULL;
    client.event_loop = NULL;
    client.rm_receiving_context = NULL;

    client.attempted_call_rpc_msg_send = client.call_rpc_msg_successfully_sent = false;
    client.reply_rpc_msg_successfully_received = false;
    client.finished = false;

    Rpc__RpcMsg *ret;

    // serialize the RpcMsg to be sent
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(call_rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(sizeof(uint8_t) * rpc_msg_size);
    rpc__rpc_msg__pack(call_rpc_msg, rpc_msg_buffer);
    client.call_rpc_msg_size = rpc_msg_size;
    client.call_rpc_msg_buffer = rpc_msg_buffer;

    // create socket
    struct addrinfo *peer = NULL;
    if (create_socket(host, port, &peer, &client) != 0) {
        ret = NULL;
        goto cleanup;
    }

    // create QUIC config
    quic_config_t *config = NULL;
    config = quic_config_new();
    if(config == NULL) {
        fprintf(stderr, "execute_rpc_call_quic: failed to create config\n");
        ret = NULL;
        goto cleanup;
    }
    quic_config_set_max_idle_timeout(config, 5000);
    quic_config_set_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);

    // create and set TLS config
    const char *const protos[1] = {"rpc"};
    client.tls_config = quic_tls_config_new_client_config(protos, 1, true);
    if(client.tls_config == NULL) {
        fprintf(stderr, "execute_rpc_call_quic: failed to create TLS config\n");
        ret = NULL;
        goto cleanup;
    }
    quic_config_set_tls_config(config, client.tls_config);

    // create QUIC endpoint
    client.quic_endpoint = quic_endpoint_new(config, false, &quic_transport_methods, &client, &quic_packet_send_methods, &client);
    if(client.quic_endpoint == NULL) {
        fprintf(stderr, "execute_rpc_call_quic: failed to create quic endpoint\n");
        ret = NULL;
        goto cleanup;
    }

    // initialize the event loop
    client.event_loop = ev_default_loop(0);
    ev_init(&client.timer, timeout_callback);
    client.timer.data = &client;

    // connect to the server
    if(quic_endpoint_connect(client.quic_endpoint, (struct sockaddr *)&client.local_addr, client.local_addr_len, peer->ai_addr, peer->ai_addrlen, NULL, NULL, 0, NULL, 0, NULL, NULL) < 0) {
        fprintf(stderr, "execute_rpc_call_quic: failed to connect to client\n");
        ret = NULL;
        goto cleanup;
    }
    process_connections(&client);

    // start the event loop
    ev_io udp_socket_watcher;
    ev_io_init(&udp_socket_watcher, read_callback, client.socket_fd, EV_READ);
    udp_socket_watcher.data = &client;
    ev_io_start(client.event_loop, &udp_socket_watcher);

    ev_run(client.event_loop, 0);

    if(!client.call_rpc_msg_successfully_sent) {
        fprintf(stderr, "execute_rpc_call_quic: failed to send RPC call message\n");
        ret = NULL;
        goto cleanup;
    }

    if(!client.reply_rpc_msg_successfully_received) {
        fprintf(stderr, "execute_rpc_call_quic: failed to receive RPC reply message\n");
        ret = NULL;
        goto cleanup;
    }

    Rpc__RpcMsg *reply_rpc_msg = deserialize_rpc_msg(client.rm_receiving_context->accumulated_payloads, client.rm_receiving_context->accumulated_payloads_size);

    ret = reply_rpc_msg;

cleanup:
    if(client.call_rpc_msg_buffer != NULL) {
        free(client.call_rpc_msg_buffer);
    }
    if(client.tls_config != NULL) {
        quic_tls_config_free(client.tls_config);
    }
    if(client.quic_endpoint != NULL) {
        quic_endpoint_free(client.quic_endpoint);
    }
    if(client.event_loop != NULL) {
        ev_loop_destroy(client.event_loop);
    }
    if(config != NULL) {
        quic_config_free(config);
    }
    free_rm_receiving_context(client.rm_receiving_context);

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

    char port_number[10];
    sprintf(port_number, "%u", rpc_connection_context->server_port);

    Rpc__RpcMsg *reply_rpc_msg = execute_rpc_call_quic(rpc_connection_context->server_ipv4_addr, port_number, &call_rpc_msg);

    return reply_rpc_msg;
}