#include "tcp_rpc_client.h"

/*
* Sends an RPC call for the given program number, program version, procedure number, and parameters,
* to the given opened TCP socket.
*
* Returns the RPC reply received from the server on success, and NULL on failure.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *execute_rpc_call_tcp(int rpc_client_socket_fd, Rpc__RpcMsg *call_rpc_msg) {
    if(call_rpc_msg == NULL) {
        return NULL;
    }

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(call_rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(sizeof(uint8_t) * rpc_msg_size);
    rpc__rpc_msg__pack(call_rpc_msg, rpc_msg_buffer);

    // send the serialized RpcMsg to the server as a single Record Marking record
    // TODO: (QNFS-37) implement time-outs + reconnections
    int error_code = send_rm_record_tcp(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    if(error_code > 0) {
        return NULL;
    }

    // receive the RPC reply from the server as a single Record Marking record
    size_t reply_rpc_msg_size = -1;
    uint8_t *reply_rpc_msg_buffer = receive_rm_record_tcp(rpc_client_socket_fd, &reply_rpc_msg_size);
    if(reply_rpc_msg_buffer == NULL) {
        return NULL;
    }

    Rpc__RpcMsg *reply_rpc_msg = deserialize_rpc_msg(reply_rpc_msg_buffer, reply_rpc_msg_size);
    free(reply_rpc_msg_buffer);

    return reply_rpc_msg;
}

/*
* Given the RPC program number to be called, program version, procedure number, and the parameters for it, calls
* the appropriate remote procedure over TCP.
*
* Returns the server's RPC reply on success, and NULL on failure.
*
* The user of this function takes the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *invoke_rpc_remote_tcp(RpcConnectionContext *rpc_connection_context, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
    // create TCP socket and verify
    int rpc_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(rpc_client_socket_fd < 0) {
        perror_msg("Socket creation failed");
        return NULL;
    }

    if(rpc_connection_context == NULL) {
        fprintf(stderr, "RpcConnectionContext is NULL\n");
        return NULL;
    }

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(rpc_connection_context->server_ipv4_addr); 
    rpc_server_addr.sin_port = htons(rpc_connection_context->server_port);

    // connect the rpc client socket to rpc server socket
    if(connect(rpc_client_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) {
        perror_msg("Connection with the server failed");
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

    Rpc__RpcMsg *reply_rpc_msg = execute_rpc_call_tcp(rpc_client_socket_fd, &call_rpc_msg);
    close(rpc_client_socket_fd);

    return reply_rpc_msg;
}