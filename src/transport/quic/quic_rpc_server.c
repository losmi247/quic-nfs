#include "quic_rpc_server.h"

/*
*  Define QUIC Nfs+Mount server state.
*/

struct QuicServer quic_server;

pthread_mutex_t quic_server_cleanup_mutex = PTHREAD_MUTEX_INITIALIZER;
bool quic_server_resources_released = false;

/*
* Sends the given ReplyBody back to the RPC client in a RpcMsg, over the stream with the given ID in the
* given QUIC connection.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_reply_body_quic(struct quic_conn_t *conn, uint64_t stream_id, Rpc__ReplyBody *reply_body) {
    Rpc__RpcMsg rpc_msg = RPC__RPC_MSG__INIT;
    rpc_msg.xid = generate_rpc_xid();
    rpc_msg.mtype = RPC__MSG_TYPE__REPLY;
    rpc_msg.body_case = RPC__RPC_MSG__BODY_RBODY; // this body_case enum is not actually sent over the network
    rpc_msg.rbody = reply_body;

    // serialize the RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(&rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(rpc_msg_size);
    rpc__rpc_msg__pack(&rpc_msg, rpc_msg_buffer);

    // send the serialized RpcMsg back to the client as a single Record Marking record
    int error_code = send_rm_record_quic(conn, stream_id, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    if(error_code > 0) {
        return 1;
    }

    return 0;
}

/*
* Sends the given AcceptedReply back to the RPC client, over the stream with the given ID in the
* given QUIC connection.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_accepted_reply_message_quic(struct quic_conn_t *conn, uint64_t stream_id, Rpc__AcceptedReply *accepted_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_ACCEPTED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_AREPLY;  // reply_case is not actually transfered over network
    reply_body.areply = accepted_reply;

    return send_rpc_reply_body_quic(conn, stream_id, &reply_body);
}

/*
* Sends the given RejectedReply back to the RPC client, over the stream with the given ID in the
* given QUIC connection.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_rejected_reply_message_quic(struct quic_conn_t *conn, uint64_t stream_id, Rpc__RejectedReply *rejected_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_DENIED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_RREPLY;  // reply_case is not actually transfered over network
    reply_body.rreply = rejected_reply;

    return send_rpc_reply_body_quic(conn, stream_id, &reply_body);
}

/*
* Prints out the given error message, and sends an AUTH_ERROR RejectedReply over the stream with the 
* given ID in the given QUIC connection.
*
* Returns 0 on success and > 0 on failure.
*/
int send_auth_error_rejected_reply_quic(struct quic_conn_t *conn, uint64_t stream_id, char *error_msg, Rpc__AuthStat auth_stat) {
    fprintf(stdout, "%s", error_msg);

    Rpc__RejectedReply *rejected_reply = create_auth_error_rejected_reply(auth_stat);

    int error_code = send_rpc_rejected_reply_message_quic(conn, stream_id, rejected_reply);
    free_rejected_reply(rejected_reply);
    if(error_code > 0) {
        fprintf(stdout, "Server failed to send AUTH_ERROR RejectedReply\n");
        return 1;
    }

    return 0;
}

/*
* Validates the 'credential' and 'verifier' OpaqueAuth pair from some RPC CallBody, to check that they have the correct
* structure (no NULL fields) and correspond to a supported authentication flavor.
*
* Returns < 0 on failure.
* On success, returns 0 if the credential and verifier pair are correct, and if the credential and verifier pair are incorrect, returns > 0
* and sends an appropriate RejectedReply over the stream with the given ID in the given QUIC connection.
*/
int validate_credential_and_verifier_quic(struct quic_conn_t *conn, uint64_t stream_id, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier) {
    int error_code;

    if(credential == NULL) {
        error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with 'credential' being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_NONE credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(credential->empty == NULL) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_NONE credential, with credential->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_AUTH_SYS) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_SYS credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }

        if(credential->auth_sys == NULL) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        Rpc__AuthSysParams *authsysparams = credential->auth_sys;

        if(authsysparams->machinename == NULL) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->machinename being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(authsysparams->gids == NULL) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->gids being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // TODO (QNFS-52): Implement AUTH_SHORT
        error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with unsupported authentication flavor %d.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }

    if(verifier == NULL) {
        error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with 'verifier' being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }
    if(verifier->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(verifier->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_NONE verifier, with inconsistent verifier->flavor and verifier->body_case.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
        if(verifier->empty == NULL) {
            error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with AUTH_NONE verifier, with verifier->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // in AUTH_NONE, AUTH_SYS, and AUTH_SHORT, verifier in CallBody always has AUTH_NONE flavor
        error_code = send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with 'verifier' having unsupported flavor.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }

    return 0;
}

/*
* Given the serialized RPC call received from a client on the given stream in the given QUIC connection,
* processes that RPC, and sends an RPC reply on the same stream.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_client_quic(uint8_t *rpc_call_buffer, size_t rpc_call_buffer_size, struct quic_conn_t *conn, uint64_t stream_id) {
    if(rpc_call_buffer == NULL) {
        return 1;
    }

    Rpc__RpcMsg *rpc_call = deserialize_rpc_msg(rpc_call_buffer, rpc_call_buffer_size);
    if(rpc_call == NULL) {
        return 2;   // invalid RPC received, no reply given
    }
    log_rpc_msg_info(rpc_call);

    if(rpc_call->mtype != RPC__MSG_TYPE__CALL || rpc_call->body_case != RPC__RPC_MSG__BODY_CBODY) {
        fprintf(stderr, "Server received an RPC reply but it should only be receiving RPC calls.\n");
        return 3;   // invalid RPC received, no reply given
    }

    Rpc__CallBody *call_body = rpc_call->cbody;
    if(call_body == NULL) {
        return 4;   // invalid RPC received, no reply given
    }

    // check RPC version
    if(call_body->rpcvers != 2) {
        fprintf(stdout, "Server received an RPC call with RPC version not equal to 2.\n");

        Rpc__RejectedReply *rejected_reply = create_rpc_mismatch_rejected_reply(2, 2);
    
        int error_code = send_rpc_rejected_reply_message_quic(conn, stream_id, rejected_reply);
        free_rejected_reply(rejected_reply);
        if(error_code > 0) {
            fprintf(stdout, "Server failed to send RPC mismatch RejectedReply\n");
            return 5;
        }

        return 0;
    }

    // check authentication fields
    int error_code = validate_credential_and_verifier_quic(conn, stream_id, call_body->credential, call_body->verifier);
    if(error_code != 0) {
        return 6;
    }
    log_rpc_call_body_info(call_body);
    if(call_body->credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE && call_body->proc != 0) {
        // only NULL procedure is allowed to use AUTH_NONE flavor
        return send_auth_error_rejected_reply_quic(conn, stream_id, "Server received an RPC call with authentication flavor AUTH_NONE for a non-NULL procedure.\n", RPC__AUTH_STAT__AUTH_TOOWEAK);
    }

    // reply with a AcceptedReply
    Google__Protobuf__Any *parameters = call_body->params;

    Rpc__AcceptedReply *accepted_reply = forward_rpc_call_to_program(call_body->credential, call_body->verifier, call_body->prog, call_body->vers, call_body->proc, parameters);
    rpc__rpc_msg__free_unpacked(rpc_call, NULL);

    error_code = send_rpc_accepted_reply_message_quic(conn, stream_id, accepted_reply);
    free_accepted_reply(accepted_reply);
    if(error_code > 0) {
        fprintf(stdout, "Server failed to send AcceptedReply\n");
        return 7;
    }

    return 0;
}

/*
* Server body implementation over QUIC.
*/

void server_on_conn_created(void *tctx, struct quic_conn_t *conn) {}

void server_on_conn_established(void *tctx, struct quic_conn_t *conn) {}

void server_on_conn_closed(void *tctx, struct quic_conn_t *conn) {}

void server_on_stream_created(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

void server_on_stream_readable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    struct QuicServer *server = tctx;

    if(find_rm_receiving_context(conn, stream_id, server->rm_receiving_contexts) > 0) {
        // this is a new stream, so create a RM receiving context for it
        int error_code = add_new_rm_receiving_context(conn, stream_id, &(server->rm_receiving_contexts));
        if(error_code > 0) {
            fprintf(stderr, "server_on_stream_readable: failed to create a RM receiving context\n");
            return;
        }
    }

    RecordMarkingReceivingContext *rm_receiving_context = get_rm_receiving_context(conn, stream_id, server->rm_receiving_contexts);
    if(rm_receiving_context == NULL) {
        fprintf(stderr, "server_on_stream_readable: failed to find the RM receiving context for the readable stream %ld\n", stream_id);
        return;
    }

    int error_code = receive_available_bytes_quic(rm_receiving_context);
    if(error_code > 0) {
        fprintf(stderr, "server_on_stream_readable: failed to read available data from the readable stream %ld\n", stream_id);
        return;
    }

    // we've received a complete RPC on this stream
    if(rm_receiving_context->record_fully_received) {
        error_code = handle_client_quic(rm_receiving_context->accumulated_payloads, rm_receiving_context->accumulated_payloads_size, conn, stream_id);
        if(error_code > 0) {
            fprintf(stderr, "failed to handle RPC call on stream %ld\n", stream_id);
        }

        remove_rm_receiving_context(conn, stream_id, &(server->rm_receiving_contexts));
    }
}

void server_on_stream_writable(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {
    quic_stream_wantwrite(conn, stream_id, false);
}

void server_on_stream_closed(void *tctx, struct quic_conn_t *conn, uint64_t stream_id) {}

/*
* Sends outbound QUIC packets via the underlying UDP socket.
*/
int server_on_packets_send(void *psctx, struct quic_packet_out_spec_t *pkts, unsigned int count) {
    struct QuicServer *server = psctx;

    unsigned int sent_count = 0;
    for(int i = 0; i < count; i++) {
        struct quic_packet_out_spec_t *pkt = pkts + i;
        for(int j = 0; j < (*pkt).iovlen; j++) {
            const struct iovec *iov = pkt->iov + j;
            ssize_t sent = sendto(server->socket_fd, iov->iov_base, iov->iov_len, 0, (struct sockaddr *)pkt->dst_addr, pkt->dst_addr_len);
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

struct quic_tls_config_t *server_get_default_tls_config(void *ctx) {
    struct QuicServer *server = ctx;
    return server->tls_config;
}

struct quic_tls_config_t *server_select_tls_config(void *ctx, const uint8_t *server_name, size_t server_name_len) {
    struct QuicServer *server = ctx;
    return server->tls_config;
}

const struct quic_transport_methods_t quic_transport_methods = {
    .on_conn_created = server_on_conn_created,
    .on_conn_established = server_on_conn_established,
    .on_conn_closed = server_on_conn_closed,
    .on_stream_created = server_on_stream_created,
    .on_stream_readable = server_on_stream_readable,
    .on_stream_writable = server_on_stream_writable,
    .on_stream_closed = server_on_stream_closed,
};

const struct quic_packet_send_methods_t quic_packet_send_methods = {
    .on_packets_send = server_on_packets_send,
};

const struct quic_tls_config_select_methods_t tls_config_select_method = {
    .get_default = server_get_default_tls_config,
    .select = server_select_tls_config,
};

/*
* Processes awaiting events on all QUIC connections at the given server's
* endpoint, advancing the QUIC state machine.
*/
static void process_connections(struct QuicServer *server) {
    quic_endpoint_process_connections(server->quic_endpoint);

    double timeout = quic_endpoint_timeout(server->quic_endpoint) / 1e3f;
    if(timeout < 0.0001) {
        timeout = 0.0001;
    }
    server->timer.repeat = timeout;
    ev_timer_again(server->event_loop, &server->timer);
}

/*
* Forwards packets received (via the underlying UDP socket) to the server's QUIC endpoint.
*/
static void read_callback(EV_P_ ev_io *w, int revents) {
    struct QuicServer *server = w->data;
    static uint8_t buf[READ_BUF_SIZE];

    while(true) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(server->socket_fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if(read < 0) {
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
            .dst = (struct sockaddr *)&server->local_addr,
            .dst_len = server->local_addr_len,
        };

        int error_code = quic_endpoint_recv(server->quic_endpoint, buf, read, &quic_packet_info);
        if(error_code != 0) {
            fprintf(stderr, "read_callback: recv failed with error %d\n", error_code);
            continue;
        }
    }

    process_connections(server);
}

static void timeout_callback(EV_P_ ev_timer *w, int revents) {
    struct QuicServer *server = w->data;
    quic_endpoint_on_timeout(server->quic_endpoint);
    process_connections(server);
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
static int create_socket(const char *host, const char *port, struct addrinfo **local, struct QuicServer *server) {
    const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
    if(getaddrinfo(host, port, &hints, local) != 0) {
        fprintf(stderr, "failed to resolve host\n");
        return 1;
    }

    int udp_socket = socket((*local)->ai_family, SOCK_DGRAM, 0);
    if(udp_socket < 0) {
        fprintf(stderr, "create_socket: failed to create socket\n");
        return 1;
    }
    if(fcntl(udp_socket, F_SETFL, O_NONBLOCK) != 0) {
        fprintf(stderr, "create_socket: failed to make socket non-blocking\n");
        return 1;
    }
    if(bind(udp_socket, (*local)->ai_addr, (*local)->ai_addrlen) < 0) {
        fprintf(stderr, "create_socket: failed to bind socket\n");
        return 1;
    }

    server->local_addr_len = sizeof(server->local_addr);
    if(getsockname(udp_socket, (struct sockaddr *)&server->local_addr, &server->local_addr_len) != 0) {
        fprintf(stderr, "create_socket: failed to get local address of socket\n");
        return 1;
    };
    server->socket_fd = udp_socket;

    return 0;
}

/*
* Frees all resources used by the given QUIC server.
*
* This function executes atomically and checks a flag that says if the resources have already
* been released. This is so that concurrent cleanups initiated from different places do not
* cause double free errors - e.g. the SIGTERM signal handler breaks the event loop, causing 
* the server process to start the cleanup of resources using this function, and then the 
* signal handler itself starts the cleanup of resources using this same function.
*
* Does nothing if the QUIC server resources have already been released.
*/
void release_quic_server_resources(struct QuicServer server) {
    pthread_mutex_lock(&quic_server_cleanup_mutex);
    if(quic_server_resources_released) {
        pthread_mutex_unlock(&quic_server_cleanup_mutex);
        return;
    }

    if(server.tls_config != NULL) {
        quic_tls_config_free(server.tls_config);
    }
    if(server.socket_fd > 0) {
        close(server.socket_fd);
    }
    if(server.quic_endpoint != NULL) {
        quic_endpoint_free(server.quic_endpoint);
    }
    if(server.event_loop != NULL) {
        ev_loop_destroy(server.event_loop);
    }
    if(server.config != NULL) {
        quic_config_free(server.config);
    }
    clean_up_rm_receiving_contexts_list(server.rm_receiving_contexts);

    quic_server_resources_released = true;
    pthread_mutex_unlock(&quic_server_cleanup_mutex);
}

/*
* Cleans up all QUIC server state.
*
* Can be called from signal handlers to ensure graceful shutdown. 
*/
void clean_up_quic_server_state(void) {
    // break the event loop
    ev_break(quic_server.event_loop, EVBREAK_ALL);

    release_quic_server_resources(quic_server);
}

/*
* Runs the Nfs+Mount server, which awaits RPCs, over QUIC.
*
* Returns > 0 on failure.
*/
int run_server_quic(uint16_t port_number) {
    quic_server.quic_endpoint = NULL;
    quic_server.config = NULL;
    quic_server.tls_config = NULL;
    quic_server.event_loop = NULL;
    quic_server.rm_receiving_contexts = NULL;

    int ret = 0;

    // create a socket
    struct addrinfo *local = NULL;
    char port[10];
    sprintf(port, "%u", port_number);
    if(create_socket("0.0.0.0", port, &local, &quic_server) > 0) {
        fprintf(stderr, "run_server_quic: failed to create a UDP socket\n");
        ret = 1;
        goto cleanup;
    }
    freeaddrinfo(local);

    // create quic config
    quic_config_t *config = quic_config_new();
    if(config == NULL) {
        ret = 1;
        goto cleanup;
    }
    quic_config_set_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quic_server.config = config;

    // create and set tls config
    const char *const protos[1] = {"rpc"};
    quic_server.tls_config = quic_tls_config_new_server_config("certificate.cert", "certificate.key", protos, 1, true);
    if(quic_server.tls_config == NULL) {
        fprintf(stderr, "run_server_quic: failed to set up TLS config\n");
        ret = 1;
        goto cleanup;
    }
    quic_config_set_tls_selector(config, &tls_config_select_method, &quic_server);

    // create quic endpoint
    quic_server.quic_endpoint = quic_endpoint_new(config, true, &quic_transport_methods, &quic_server, &quic_packet_send_methods, &quic_server);
    if(quic_server.quic_endpoint == NULL) {
        fprintf(stderr, "run_server_quic: failed to create quic endpoint\n");
        ret = 1;
        goto cleanup;
    }

    // start the event loop
    quic_server.event_loop = ev_default_loop(0);
    ev_init(&quic_server.timer, timeout_callback);
    quic_server.timer.data = &quic_server;

    fprintf(stdout, "Server listening on port %d... (QUIC)\n", port_number);

    ev_io watcher;
    ev_io_init(&watcher, read_callback, quic_server.socket_fd, EV_READ);
    ev_io_start(quic_server.event_loop, &watcher);
    watcher.data = &quic_server;
    ev_run(quic_server.event_loop, 0);

cleanup:
    release_quic_server_resources(quic_server);

    return ret;
}