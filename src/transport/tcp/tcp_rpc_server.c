#include "tcp_rpc_server.h"

#include "src/nfs/server/server.h"

#include "src/nfs/server/nfs_server_threads.h"

/*
*  Define TCP Nfs+Mount server state.
*/

int rpc_server_socket_fd;

pthread_mutex_t tcp_server_cleanup_mutex = PTHREAD_MUTEX_INITIALIZER;
bool tcp_server_resources_released = false;

/*
* Sends the given ReplyBody back to the RPC client in a RpcMsg, given the opened TCP socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_reply_body_tcp(int rpc_client_socket_fd, Rpc__ReplyBody *reply_body) {
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
    int error_code = send_rm_record_tcp(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    if(error_code > 0) {
        return 1;
    }

    return 0;
}

/*
* Sends the given AcceptedReply back to the RPC client, given the opened TCP socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_accepted_reply_message_tcp(int rpc_client_socket_fd, Rpc__AcceptedReply *accepted_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_ACCEPTED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_AREPLY;  // reply_case is not actually transfered over network
    reply_body.areply = accepted_reply;

    return send_rpc_reply_body_tcp(rpc_client_socket_fd, &reply_body);
}

/*
* Sends the given RejectedReply back to the RPC client, given the opened TCP socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_rejected_reply_message_tcp(int rpc_client_socket_fd, Rpc__RejectedReply *rejected_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_DENIED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_RREPLY;  // reply_case is not actually transfered over network
    reply_body.rreply = rejected_reply;

    return send_rpc_reply_body_tcp(rpc_client_socket_fd, &reply_body);
}

/*
* Prints out the given error message, and sends an AUTH_ERROR RejectedReply with the given AuthStat to the 
* given opened TCP client socket.
*
* Returns 0 on success and > 0 on failure.
*/
int send_auth_error_rejected_reply_tcp(int rpc_client_socket_fd, char *error_msg, Rpc__AuthStat auth_stat) {
    fprintf(stdout, "%s", error_msg);

    Rpc__RejectedReply *rejected_reply = create_auth_error_rejected_reply(auth_stat);

    int error_code = send_rpc_rejected_reply_message_tcp(rpc_client_socket_fd, rejected_reply);
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
* and sends an appropriate RejectedReply to the given opened TCP client socket.
*/
int validate_credential_and_verifier_tcp(int rpc_client_socket_fd, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier) {
    int error_code;

    if(credential == NULL) {
        error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with 'credential' being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(credential->empty == NULL) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE credential, with credential->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_AUTH_SYS) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }

        if(credential->auth_sys == NULL) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        Rpc__AuthSysParams *authsysparams = credential->auth_sys;

        if(authsysparams->machinename == NULL) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->machinename being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(authsysparams->gids == NULL) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->gids being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // TODO (QNFS-52): Implement AUTH_SHORT
        error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with unsupported authentication flavor %d.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }

    if(verifier == NULL) {
        error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with 'verifier' being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }
    if(verifier->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(verifier->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE verifier, with inconsistent verifier->flavor and verifier->body_case.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
        if(verifier->empty == NULL) {
            error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE verifier, with verifier->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // in AUTH_NONE, AUTH_SYS, and AUTH_SHORT, verifier in CallBody always has AUTH_NONE flavor
        error_code = send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with 'verifier' having unsupported flavor.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }

    return 0;
}

/*
* Takes an opened TCP client socket and reads and processes a single RPC from it.
*
* Returns 0 on success and > 0 on failure.
*/
int process_single_rpc_tcp(int rpc_client_socket_fd) {
    // read one RPC call as a single Record Marking record
    size_t rpc_msg_size = -1;
    uint8_t *rpc_msg_buffer = receive_rm_record_tcp(rpc_client_socket_fd, &rpc_msg_size);
    if(rpc_msg_buffer == NULL) {
        return 1;   // failed to receive the RPC, no reply given
    }

    Rpc__RpcMsg *rpc_call = deserialize_rpc_msg(rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
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
    
        int error_code = send_rpc_rejected_reply_message_tcp(rpc_client_socket_fd, rejected_reply);
        free_rejected_reply(rejected_reply);
        if(error_code > 0) {
            fprintf(stdout, "Server failed to send RPC mismatch RejectedReply\n");
            return 5;
        }

        return 0;
    }

    // check authentication fields
    int error_code = validate_credential_and_verifier_tcp(rpc_client_socket_fd, call_body->credential, call_body->verifier);
    if(error_code != 0) {
        return 6;
    }
    log_rpc_call_body_info(call_body);
    if(call_body->credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE && call_body->proc != 0) {
        // only NULL procedure is allowed to use AUTH_NONE flavor
        return send_auth_error_rejected_reply_tcp(rpc_client_socket_fd, "Server received an RPC call with authentication flavor AUTH_NONE for a non-NULL procedure.\n", RPC__AUTH_STAT__AUTH_TOOWEAK);
    }

    // reply with a AcceptedReply
    Google__Protobuf__Any *parameters = call_body->params;

    Rpc__AcceptedReply *accepted_reply = forward_rpc_call_to_program(call_body->credential, call_body->verifier, call_body->prog, call_body->vers, call_body->proc, parameters);
    rpc__rpc_msg__free_unpacked(rpc_call, NULL);

    error_code = send_rpc_accepted_reply_message_tcp(rpc_client_socket_fd, accepted_reply);
    free_accepted_reply(accepted_reply);
    if(error_code > 0) {
        fprintf(stdout, "Server failed to send AcceptedReply\n");
        return 7;
    }

    return 0;
}

/*
* Returns 0 if the client-side socket that was connected to the given rpc_client_socket_fd
* has been closed using close() (i.e. the TCP connection has been closed).
*
* Returns < 0 if an error occurred, and 1 if the TCP connection is still alive.
*/
int is_tcp_connection_closed(int rpc_client_socket_fd) {
    // peek at the first byte to be read (blocks if connection alive and no data to be read yet)
    char peek_buffer;
    ssize_t peek_res = recv(rpc_client_socket_fd, &peek_buffer, 1, MSG_PEEK);
    if(peek_res < 0) {
        return peek_res;
    }
    
    if(peek_res == 0) {
        return 0;
    }

    return 1;
}

/*
* Function for a single NFS thread to wait for client RPCs and respond to them.
*/
void *handle_client_tcp(void *arg) {
    pthread_t tid = pthread_self();

    int *rpc_client_socket_fd = (int *) arg;
    if(rpc_client_socket_fd == NULL) {
        fprintf(stderr, "handle_client_tcp: server thread received a NULL client socket\n");

        remove_server_thread(tid, &nfs_server_threads_list);

        return NULL;
    }

    while(1) {
        int status = is_tcp_connection_closed(*rpc_client_socket_fd);
        if(status < 0) {
            perror_msg("handle_client_tcp: server thread failed to check if the TCP connection is alive\n");

            remove_server_thread(tid, &nfs_server_threads_list);

            return NULL;
        }
        else if(status == 0) {
            // client-side socket has been closed, so terminate this server thread
            return NULL;
        }

        int error_code = process_single_rpc_tcp(*rpc_client_socket_fd);
        if(error_code > 0) {
            fprintf(stderr, "handle_client_tcp: server thread failed to process a RPC with status %d\n", error_code);

            remove_server_thread(tid, &nfs_server_threads_list);

            return NULL;
        }
    }

    return NULL;
}

/*
* Frees all resources used by the TCP server.
*
* This function executes atomically and checks a flag that says if the 
* resources have already been released, so that concurrent cleanups
* initiated from different places do not cause double free errors.
*
* Does nothing if the QUIC server resources have already been released.
*/
void release_tcp_server_resources(void) {
    pthread_mutex_lock(&tcp_server_cleanup_mutex);
    if(tcp_server_resources_released) {
        pthread_mutex_unlock(&tcp_server_cleanup_mutex);
        return;
    }

    close(rpc_server_socket_fd);

    tcp_server_resources_released = true;
    pthread_mutex_unlock(&tcp_server_cleanup_mutex);
}

/*
* Cleans up all TCP server state.
*
* Can be called from signal handlers to ensure graceful shutdown.
*/
void clean_up_tcp_server_state(void) {
    release_tcp_server_resources();
}

/*
* Runs the Nfs+Mount server, which awaits RPCs, over TCP.
*
* Returns > 0 on failure.
*/
int run_server_tcp(uint16_t port_number) {
    // create the server socket
    rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(rpc_server_socket_fd < 0) { 
        fprintf(stderr, "run_server_tcp: socket creation failed\n");
        return 1;
    }

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET; 
    rpc_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rpc_server_addr.sin_port = htons(port_number);
  
    // bind socket to the rpc server address
    if(bind(rpc_server_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) { 
        fprintf(stderr, "run_server_tcp: socket bind failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    // listen for connections on the port
    if(listen(rpc_server_socket_fd, 10) < 0) {
        fprintf(stderr, "run_server_tcp: listen failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }

    fprintf(stdout, "Server listening on port %d... (TCP)\n", port_number);
  
    while(1) {
        struct sockaddr_in rpc_client_addr;
        socklen_t rpc_client_addr_len = sizeof(rpc_client_addr);

        int *rpc_client_socket_fd = malloc(sizeof(int));
        if(rpc_client_socket_fd == NULL) {
            fprintf(stderr, "run_server_tcp: server failed to allocate memory\n");
            break;
        }
        *rpc_client_socket_fd = accept(rpc_server_socket_fd, (struct sockaddr *) &rpc_client_addr, &rpc_client_addr_len);
        if(*rpc_client_socket_fd < 0) {
            fprintf(stderr, "run_server_tcp: server failed to accept connection\n");

            free(rpc_client_socket_fd);

            break;
        }

        // start a new thread for handling this client
        pthread_t server_thread;
        if(pthread_create(&server_thread, NULL, handle_client_tcp, rpc_client_socket_fd) != 0) {
            fprintf(stderr, "run_server_tcp: server failed to create a new client-handling thread\n");

            close(*rpc_client_socket_fd);
            free(rpc_client_socket_fd);

            break;
        }

        TransportConnection transport_connection;
        transport_connection.tcp_rpc_client_socket_fd = rpc_client_socket_fd;
        int error_code = add_server_thread(server_thread, TRANSPORT_PROTOCOL_TCP, transport_connection, &nfs_server_threads_list);
        if(error_code > 0) {
            fprintf(stderr, "run_server_tcp: server failed to add a new entry in the NFS server threads list\n");

            close(*rpc_client_socket_fd);
            free(rpc_client_socket_fd);

            pthread_cancel(server_thread);
            pthread_join(server_thread, NULL);

            break;
        }
    }
     
    clean_up_tcp_server_state();

    return 0;
}