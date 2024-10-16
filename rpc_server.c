#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()

#include <protobuf-c/protobuf-c.h>
#include "serialization/rpc/rpc.pb-c.h"
#include "serialization/mount/mount.pb-c.h"

#include "rpc.h"

#include "rpc_programs/mount/caller.h";

#define RPC_CALL_BUFFER_SIZE 500

Rpc__RpcMsg *deserialize_rpc_msg(uint8_t *rpc_msg_buffer, size_t bytes_received) {
    Rpc__RpcMsg *rpc_msg = rpc__rpc_msg__unpack(NULL, bytes_received, rpc_msg_buffer);
    if(rpc_msg == NULL) {
        fprintf(stderr, "Error unpacking received message");
        return NULL;
    }

    printf("xid: %u\n", rpc_msg->xid);
    printf("message type: %d\n", rpc_msg->mtype);
    if(rpc_msg->mtype != RPC__MSG_TYPE__CALL) {
        frpintf(stderr, "Server is only supposed to receiver RPC calls but it received a RPC reply");
        return NULL;
    }
    printf("body case: %d\n", rpc_msg->body_case);

    return rpc_msg;
}

Google__Protobuf__Any *forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    printf("Program number: %d\n", program_number);
    printf("Program version: %d\n", program_version);
    printf("Procedure number: %d\n", procedure_number);

    switch(program_number) {
        case 10003:
            //return call_nfs(program_version, procedure_number, parameters);
        case 10005:
            return call_mount(program_version, procedure_number, parameters);
        default:
            fprintf(stderr, "Unknown program number");
            return NULL;
    }
}

uint32_t generate_rpc_xid() {
    return rand();
}

/*
* Sends an accepted RPC Reply back to the RPC client.
* Returns 0 on successful send of a reply, and > 0 on error.
*
* If AcceptStat is PROG_MISMATCH (requested program version not supported), set non-NULL mismatch-info when calling.
* Otherwise (e.g. if AcceptStat is SUCCESS or anything else), leave mismatch_info NULL when invoking.
*/
int send_rpc_accepted_reply_message(int rpc_client_socket_fd, Rpc__ReplyStat reply_stat, Rpc__AcceptStat accept_stat, Rpc__MismatchInfo *mismatch_info ,Google__Protobuf__Any procedure_results) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = accept_stat;
    switch(accept_stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS; // reply_data_case is not actually sent over network
            if(mismatch_info != NULL) {
                return 1;
            }
            accepted_reply.results = &procedure_results;
        case RPC__ACCEPT_STAT__PROG_MISMATCH:
            accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO;
            if(mismatch_info == NULL) {
                return 1;
            }
            accepted_reply.mismatch_info = mismatch_info;
        default:
            accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
            if(mismatch_info != NULL) {
                return 1;
            }
            // I don't have to set accepted_reply->default_case Empty?
    }

    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = reply_stat;
    if(reply_stat != RPC__REPLY_STAT__MSG_ACCEPTED) {
        return 2;
    }
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_AREPLY;  // reply_case is not actually transfered over network
    reply_body.areply = &accepted_reply;

    Rpc__RpcMsg rpc_msg = RPC__RPC_MSG__INIT;
    rpc_msg.xid = generate_rpc_xid();
    rpc_msg.mtype = RPC__MSG_TYPE__REPLY;
    rpc_msg.body_case = RPC__RPC_MSG__BODY_RBODY; // this body_case enum is not actually sent over the network
    rpc_msg.rbody = &reply_body;

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(&rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(rpc_msg_size);
    rpc__rpc_msg__pack(&rpc_msg, rpc_msg_buffer);

    // send serialized RpcMsg back to the client over network
    write(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);

    free(rpc_msg_buffer);

    return 0;
}

/*
* Sends a rejected RPC Reply back to the RPC client.
* Returns 0 on successful send of a reply, and > 0 on error.
*
* If RejectStat is RPC_MISMATCH (requested rpc version not supported), set non-NULL mismatch-info when calling, and NULL for auth_stat.
* If RejectStat is AUTH_ERROR (rpc auth failed), leave mismatch_info NULL when invoking, and set non-NULL auth_stat.
*/
int send_rpc_rejected_reply_message(int rpc_client_socket_fd, Rpc__ReplyStat reply_stat, Rpc__RejectStat reject_stat, Rpc__MismatchInfo *mismatch_info, Rpc__AuthStat *auth_stat) {
    Rpc__RejectedReply rejected_reply = RPC__REJECTED_REPLY__INIT;
    rejected_reply.stat = reject_stat;
    switch(reject_stat) {
        case RPC__REJECT_STAT__RPC_MISMATCH:
            rejected_reply.reply_data_case = RPC__REJECTED_REPLY__REPLY_DATA_MISMATCH_INFO; // reply_data_case is not actually sent over network
            if(mismatch_info == NULL || auth_stat != NULL) {
                return 1;
            }
            rejected_reply.mismatch_info = mismatch_info;
        case RPC__REJECT_STAT__AUTH_ERROR:
            rejected_reply.reply_data_case = RPC__REJECTED_REPLY__REPLY_DATA_AUTH_STAT;
            if(mismatch_info != NULL || auth_stat == NULL) {
                return 1;
            }
            rejected_reply.auth_stat = *auth_stat;
        default:
            return 2; // reject_stat can't be anything other that the two cases above
    }

    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = reply_stat;
    if(reply_stat != RPC__REPLY_STAT__MSG_DENIED) {
        return 3;
    }
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_RREPLY;  // reply_case is not actually transfered over network
    reply_body.rreply = &rejected_reply;

    Rpc__RpcMsg rpc_msg = RPC__RPC_MSG__INIT;
    rpc_msg.xid = generate_rpc_xid();
    rpc_msg.mtype = RPC__MSG_TYPE__REPLY;
    rpc_msg.body_case = RPC__RPC_MSG__BODY_RBODY; // this body_case enum is not actually sent over the network
    rpc_msg.rbody = &reply_body;

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(&rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(rpc_msg_size);
    rpc__rpc_msg__pack(&rpc_msg, rpc_msg_buffer);

    // send serialized RpcMsg back to the client over network
    write(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);

    free(rpc_msg_buffer);

    return 0;
}

void handle_client(int rpc_client_socket_fd) {
    uint8_t *rpc_msg_buffer = (uint8_t *) malloc(RPC_CALL_BUFFER_SIZE * sizeof(uint8_t));
    size_t bytes_received = recv(rpc_client_socket_fd, rpc_msg_buffer, RPC_CALL_BUFFER_SIZE, 0);

    Rpc__RpcMsg *rpc_msg = deserialize_rpc_msg(rpc_msg_buffer, bytes_received);
    if(rpc_msg == NULL) {
        // here send a RPC reply with appropriate status code saying what went wrong
        return;
    }

    Rpc__CallBody *call_body = rpc_msg->cbody;
    if(call_body == NULL) {
        // here send a RPC reply with appropriate status code saying what went wrong (rejected_reply)
        return;
    }
    if(call_body->rpcvers != 2) {
        fprintf(stderr, "Server got an RPC call with RPC version not equal to 2");

        Rpc__MismatchInfo mismatch_info = RPC__MISMATCH_INFO__INIT;
        mismatch_info.low = 2;
        mismatch_info.high = 2;
        send_rpc_rejected_reply_message(rpc_client_socket_fd, RPC__REPLY_STAT__MSG_DENIED, RPC__REJECT_STAT__RPC_MISMATCH, &mismatch_info, NULL);

        return;
    }
    Google__Protobuf__Any *parameters = call_body->params;

    Google__Protobuf__Any *procedure_results = forward_rpc_call_to_program(call_body->prog, call_body->vers, call_body->proc, parameters);
    // if procedure_results != NULL, here send a RPC reply with the procedure results (accepted_reply)
    // if procedure_results == NULL, here send a RPC reply with appropriate status code saying program not found (rejected_reply)
    
    rpc__rpc_msg__free_unpacked(rpc_msg, NULL);

/*
    // Deserialize the buffer back into a FhStatus message
    Nfs__Mount__FhStatus *fh_status = nfs__mount__fh_status__unpack(NULL, bytes_received, buffer);
    if (fh_status == NULL) {
        fprintf(stderr, "Error unpacking received message\n");
        exit(EXIT_FAILURE);
    }
    // Access the deserialized fields
    printf("Status: %u\n", fh_status->status);
    if (fh_status->status == 0 && fh_status->directory != NULL) {
        printf("Directory Handle Length: %lu\n", fh_status->directory->handle.len);
        printf("Directory Handle Data: %.*s\n", (int)fh_status->directory->handle.len, fh_status->directory->handle.data);
    }
    else {
        printf("No valid directory handle received (status: %u)\n", fh_status->status);
    }

    // Free the allocated message
    nfs__mount__fh_status__free_unpacked(fh_status, NULL);
    */

    /*
    char *response = (char *) malloc(BUFFER_SIZE * sizeof(char));
    snprintf(response, BUFFER_SIZE, "hello");
    send(client_socket_fd, response, BUFFER_SIZE, 0);
    */

    free(rpc_msg_buffer);
}

int main() {
    int rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(rpc_server_socket_fd < 0) { 
        perror("Socket creation failed");
        return 1;
    } 

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET; 
    rpc_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rpc_server_addr.sin_port = htons(RPC_SERVER_PORT);
  
    // bind socket to the rpc server address
    if(bind(rpc_server_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) { 
        perror("Socket bind failed");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    // listen for connections on the port
    if(listen(rpc_server_socket_fd, 10) < 0) {
        perror("Listen failed");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    while(1) {
        struct sockaddr_in rpc_client_addr;
        socklen_t rpc_client_addr_len = sizeof(rpc_client_addr);

        int rpc_client_socket_fd = accept(rpc_server_socket_fd, (struct sockaddr *) &rpc_client_addr, &rpc_client_addr_len);
        if(rpc_client_socket_fd < 0) { 
            perror("Server failed to accept connection"); 
            continue;
        }
    
        handle_client(rpc_client_socket_fd);

        close(rpc_client_socket_fd);
    }
     
    close(rpc_server_socket_fd);
} 