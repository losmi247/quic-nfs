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

#include "common_rpc.h"
#include "client_common_rpc.h"

/*
* Sends an RPC to the given socket.
*/
Rpc__RpcMsg *send_rpc_call(int rpc_client_socket_fd, uint32_t program_number, uint32_t program_version, uint32_t procedure_version, Google__Protobuf__Any parameters) {
    Rpc__CallBody call_body = RPC__CALL_BODY__INIT;
    call_body.rpcvers = 2;
    call_body.prog = program_number;
    call_body.vers = program_version;
    call_body.proc = procedure_version;
    call_body.params = &parameters;

    Rpc__RpcMsg rpc_msg = RPC__RPC_MSG__INIT;
    rpc_msg.xid = generate_rpc_xid();
    rpc_msg.mtype = RPC__MSG_TYPE__CALL;
    rpc_msg.body_case = RPC__RPC_MSG__BODY_CBODY; // this body_case enum is not actually sent over the network
    rpc_msg.cbody = &call_body;

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(&rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(rpc_msg_size);
    rpc__rpc_msg__pack(&rpc_msg, rpc_msg_buffer);

    // send serialized RpcMsg to server over network
    // need to implement time-outs + reconnections in case the server doesn't respond some time after we sent the RPC call
    // for now just wait for the reply to be delivered to your socket - file descriptors are blocking by default so 'recv' sleeps waiting for a message
    write(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    uint8_t *rpc_reply_buffer = (uint8_t *) malloc(RPC_MSG_BUFFER_SIZE * sizeof(uint8_t));
    size_t bytes_received = recv(rpc_client_socket_fd, rpc_reply_buffer, RPC_MSG_BUFFER_SIZE, 0);

    Rpc__RpcMsg *rpc_reply = deserialize_rpc_msg(rpc_reply_buffer, bytes_received);
    free(rpc_reply_buffer);

    return rpc_reply;
}

/*
* Given the RPC message received from the server, returns the procedure results from it.
* 
* If the RPC message has incosistencies the errors are printed to stderr and NULL is returned,
* otherwise the RPC message is a valid RPC reply.
* If the RPC reply is a RejectedReply, this is printed to stdout and NULL is returned.
* Otherwise the RPC reply is an AcceptedReply, and procedure results are returned.
*/
Google__Protobuf__Any *extract_procedure_results_from_rpc_reply(Rpc__RpcMsg *rpc_reply) {
    if(rpc_reply == NULL) {
        fprintf(stderr, "Something went wrong at server - NULL rpc_reply in received RPC reply.");
        return NULL;
    }

    // receiving a RPC call at client means something went wrong
    if(rpc_reply->mtype != RPC__MSG_TYPE__REPLY || rpc_reply->body_case != RPC__RPC_MSG__BODY_RBODY) {
        fprintf(stderr, "Client received an RPC call but it should only be receiving RPC replies.");
        return NULL;
    }

    Rpc__ReplyBody *reply_body = rpc_reply->rbody;
    if(reply_body == NULL) {
        fprintf(stderr, "Something went wrong at server - NULL reply_body in received RPC reply.");
        return NULL;
    }

    if(reply_body->stat == RPC__REPLY_STAT__MSG_DENIED) {
        if(reply_body->reply_case != RPC__REPLY_BODY__REPLY_RREPLY) {
            fprintf(stderr, "Something went wrong at server - mismatch between ReplyStat and reply_case in received RPC reply.");
            return NULL;
        }

        Rpc__RejectedReply *rejected_reply = reply_body->rreply;
        if(rejected_reply == NULL) {
            fprintf(stderr, "Something went wrong at server - NULL rejected_reply in received RPC reply.");
            return NULL;         
        }

        switch(rejected_reply->stat) {
            case RPC__REJECT_STAT__RPC_MISMATCH:
                if(rejected_reply->reply_data_case != RPC__REJECTED_REPLY__REPLY_DATA_MISMATCH_INFO) {
                    fprintf(stderr, "Something went wrong at server - mismatch between RejectStat and reply_data_case in received RPC reply.");
                    return NULL;
                }
                
                Rpc__MismatchInfo *mismatch_info = rejected_reply->mismatch_info;
                if(mismatch_info == NULL) {
                    fprintf(stderr, "Something went wrong at server - NULL mismatch_info in received RPC reply.");
                    return NULL;
                }

                fprintf(stdout, "Rejected RPC Reply: requested RPC version not supported, min version=%d, max version=%d", mismatch_info->low, mismatch_info->high);
                return NULL;

            case RPC__REJECT_STAT__AUTH_ERROR:
                if(rejected_reply->reply_data_case != RPC__REJECTED_REPLY__REPLY_DATA_AUTH_STAT) {
                    fprintf(stderr, "Something went wrong at server - mismatch between RejectStat and reply_data_case in received RPC reply.");
                    return NULL;
                }

                fprintf(stdout, "Rejected RPC Reply: authentication failed with AuthStat %d", rejected_reply->auth_stat);
                return NULL;

            // no other RejectStat exists
        }
    }

    // now the reply must be AcceptedReply
    if(reply_body->stat != RPC__REPLY_STAT__MSG_ACCEPTED || reply_body->reply_case != RPC__REPLY_BODY__REPLY_AREPLY) {
        // something went wrong at server - mismatch between ReplyStat and reply_case
        return NULL;
    }

    Rpc__AcceptedReply *accepted_reply = reply_body->areply;

    switch(accepted_reply->stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS) {
                fprintf(stderr, "Something went wrong at server - mismatch between AcceptStat and reply_data_case in received RPC reply.");
                return NULL;
            }

            Google__Protobuf__Any *procedure_results = accepted_reply->results;
            if(procedure_results == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL results in received RPC reply.");
                return NULL;
            }

            return procedure_results;
        case RPC__ACCEPT_STAT__PROG_MISMATCH:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO) {
                fprintf(stderr, "Something went wrong at server - mismatch between AcceptStat and reply_data_case in received RPC reply.");
                return NULL;
            }

            Rpc__MismatchInfo *mismatch_info = accepted_reply->mismatch_info;
            if(mismatch_info == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL mismatch_info in received RPC reply.");
                return NULL;
            }

            fprintf(stdout, "Rejected RPC Reply: requested RPC program version not supported, min version=%d, max version=%d", mismatch_info->low, mismatch_info->high);
            return NULL;
        default:
            fprintf(stdout, "Accepted RPC Reply with AcceptStat: %d", accepted_reply->stat);
            return NULL;
    }
}

/*
* Takes in the RPC program number to be called, program version, procedure number, and the parameters for it.
* After calling the procedure, returns the server's response.
*/
Google__Protobuf__Any *invoke_rpc(const char *server_ipv4_addr, uint16_t server_port, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
    // create TCP socket and verify
    int rpc_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(rpc_client_socket_fd < 0) {
        fprintf(stderr, "Socket creation failed.");
        return NULL;
    }

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(server_ipv4_addr); 
    rpc_server_addr.sin_port = htons(server_port);

    // connect the rpc client socket to rpc server socket
    if(connect(rpc_client_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) {
        fprintf(stderr, "Connection with the server failed.");
        return NULL;
    }

    Rpc__RpcMsg *rpc_reply = send_rpc_call(rpc_client_socket_fd, program_number, program_version, procedure_number, parameters);
    
    Google__Protobuf__Any *procedure_results = process_reply_body(rpc_reply);

    rpc__rpc_msg__free_unpacked(rpc_reply, NULL);
    close(rpc_client_socket_fd);

    return procedure_results;
}

/*
void send_data(int client_socket_fd) {
    // Initialize a FhStatus message
    Nfs__Mount__FhStatus fh_status = NFS__MOUNT__FH_STATUS__INIT;
    fh_status.status = 0; // Success

    // Initialize a FHandle message
    Nfs__Mount__FHandle fh_handle = NFS__MOUNT__FHANDLE__INIT;
    const char *sample_handle = "sample_handle";
    fh_handle.handle.len = strlen(sample_handle); 
    fh_handle.handle.data = (uint8_t *) malloc(fh_handle.handle.len);
    memcpy(fh_handle.handle.data, sample_handle, fh_handle.handle.len); // Copy the sample handle to the data

    // Assign the FHandle to fh_status
    fh_status.directory = malloc(sizeof(Nfs__Mount__FHandle)); // Allocate memory for the FHandle pointer
    if (fh_status.directory == NULL) {
        fprintf(stderr, "Memory allocation for directory failed\n");
        free(fh_handle.handle.data); // Clean up previously allocated data
        exit(EXIT_FAILURE);
    }
    // Copy the FHandle data into the directory
    memcpy(fh_status.directory, &fh_handle, sizeof(Nfs__Mount__FHandle));

    // Serialize the message
    size_t size = nfs__mount__fh_status__get_packed_size(&fh_status);
    uint8_t *buffer = malloc(size);
    nfs__mount__fh_status__pack(&fh_status, buffer);

    // The 'buffer' now contains the serialized data - send it to server
    write(client_socket_fd, buffer, size);

    // Clean up
    free(fh_status.directory); // Free the allocated directory (FHandle)
    free(fh_handle.handle.data); // Free the allocated data for FHandle
    free(buffer);
}

int main() {
    // example of what the user (e.g. NFS client implementation) of this client will do:

    Nfs__Mount__DirPath dirpath = NFS__MOUNT__DIR_PATH__INIT;
    dirpath.path = "some/directory/path";
    // Serialize the DirPath message
    size_t dirpath_size = nfs__mount__dir_path__get_packed_size(&dirpath);
    uint8_t *dirpath_buffer = malloc(dirpath_size);
    nfs__mount__dir_path__pack(&dirpath, dirpath_buffer);

    // prepare the Any message to wrap DirPath
    Google__Protobuf__Any params = GOOGLE__PROTOBUF__ANY__INIT;
    params.type_url = "nfs.mount/DirPath"; // This should match your package and message name
    params.value.data = dirpath_buffer;
    params.value.len = dirpath_size;

    Google__Protobuf__Any *results = invoke_rpc("127.0.0.1",10005, 2, 1, params);
    // deserialize params here (not the job of RPC client)

    free(dirpath_buffer);

    //send_data(client_socket_fd);
    /*char *buff = (char *) malloc(200);
    snprintf(buff, 200, "hello I'm client");
    write(client_socket_fd, buff, 200);

    recv(client_socket_fd, buff, 200, 0);
    printf("Received from server: %s\n", buff);
    free(buff);
}

*/