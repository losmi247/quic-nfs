#include <stdio.h> 
#include <netdb.h>
#include <arpa/inet.h> // inet_addr()
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> // read(), write(), close()

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"

#include "common_rpc.h"
#include "client_common_rpc.h"

/*
* Sends an RPC to the given socket.
* Returns the RPC reply received from the server.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
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
* Takes in the RPC program number to be called, program version, procedure number, and the parameters for it.
* After calling the procedure, returns the server's RPC reply.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *invoke_rpc(const char *server_ipv4_addr, uint16_t server_port, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
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
    close(rpc_client_socket_fd);

    return rpc_reply;
}

/*
* Checks the RPC message received from the server is a valid RPC reply.
* 
* If the RPC message is a valid RPC reply, 0 is returned.
* If the RPC message has incosistencies the errors are printed to stderr and error code > 0 is returned.
*/
int check_rpc_msg_is_valid_rpc_reply(Rpc__RpcMsg *rpc_reply) {
    if(rpc_reply == NULL) {
        fprintf(stderr, "Something went wrong at server - NULL rpc_reply in received RPC reply.\n");
        return 1;
    }

    // receiving a RPC call at client means something went wrong
    if(rpc_reply->mtype != RPC__MSG_TYPE__REPLY || rpc_reply->body_case != RPC__RPC_MSG__BODY_RBODY) {
        fprintf(stderr, "Client received an RPC call but it should only be receiving RPC replies.\n");
        return 2;
    }

    Rpc__ReplyBody *reply_body = rpc_reply->rbody;
    if(reply_body == NULL) {
        fprintf(stderr, "Something went wrong at server - NULL reply_body in received RPC reply.\n");
        return 3;
    }

    if(reply_body->stat == RPC__REPLY_STAT__MSG_DENIED) {
        if(reply_body->reply_case != RPC__REPLY_BODY__REPLY_RREPLY) {
            fprintf(stderr, "Something went wrong at server - inconsistent ReplyStat and reply_case in received RPC reply.\n");
            return 4;
        }

        Rpc__RejectedReply *rejected_reply = reply_body->rreply;
        if(rejected_reply == NULL) {
            fprintf(stderr, "Something went wrong at server - NULL rejected_reply in received RPC reply.\n");
            return 5;         
        }

        switch(rejected_reply->stat) {
            case RPC__REJECT_STAT__RPC_MISMATCH:
                if(rejected_reply->reply_data_case != RPC__REJECTED_REPLY__REPLY_DATA_MISMATCH_INFO) {
                    fprintf(stderr, "Something went wrong at server - inconsistent RejectStat and reply_data_case in received RPC reply.\n");
                    return 6;
                }
                
                Rpc__MismatchInfo *mismatch_info = rejected_reply->mismatch_info;
                if(mismatch_info == NULL) {
                    fprintf(stderr, "Something went wrong at server - NULL mismatch_info in received RPC reply.\n");
                    return 7;
                }

                return 0;

            case RPC__REJECT_STAT__AUTH_ERROR:
                if(rejected_reply->reply_data_case != RPC__REJECTED_REPLY__REPLY_DATA_AUTH_STAT) {
                    fprintf(stderr, "Something went wrong at server - inconsistent RejectStat and reply_data_case in received RPC reply.\n");
                    return 8;
                }

                return 0;

            // no other RejectStat exists
        }
    }

    // now the reply must be AcceptedReply
    if(reply_body->stat != RPC__REPLY_STAT__MSG_ACCEPTED || reply_body->reply_case != RPC__REPLY_BODY__REPLY_AREPLY) {
        fprintf(stderr, "Something went wrong at server - inconsistent ReplyStat and reply_case.\n");
        return 9;
    }

    Rpc__AcceptedReply *accepted_reply = reply_body->areply;

    switch(accepted_reply->stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 10;
            }

            Google__Protobuf__Any *procedure_results = accepted_reply->results;
            if(procedure_results == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL results in received RPC reply.\n");
                return 11;
            }

            return 0;
        case RPC__ACCEPT_STAT__PROG_MISMATCH:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 12;
            }

            Rpc__MismatchInfo *mismatch_info = accepted_reply->mismatch_info;
            if(mismatch_info == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL mismatch_info in received RPC reply.\n");
                return 13;
            }

            return 0;
        default:
            return 0;
    }
}

/*
* This function can only be used after the rpc_reply has been successfully
* validated with the 'check_rpc_msg_is_valid_rpc_reply' function.
*
* If the RPC reply is RejectedReply, an appropriate message is printed to 'stdout' and error code > 0 is returned.
* If the RPC reply is AcceptedReply, for SUCCESS AcceptStat we return zero, and for other AcceptStat we print
* appopriate message to 'stdout' and return error code > 0.
*
* The error codes in this function start from 14, continuing from the 'check_rpc_msg_is_valid_rpc_reply' function.
*/
int check_valid_rpc_reply_is_AcceptedReply_with_success_AcceptStat(Rpc__RpcMsg *rpc_reply) {
    Rpc__ReplyBody *reply_body = rpc_reply->rbody;

    if(reply_body->stat == RPC__REPLY_STAT__MSG_DENIED) {
        Rpc__RejectedReply *rejected_reply = reply_body->rreply;

        switch(rejected_reply->stat) {
            case RPC__REJECT_STAT__RPC_MISMATCH:
                Rpc__MismatchInfo *mismatch_info = rejected_reply->mismatch_info;
                fprintf(stdout, "Rejected RPC Reply: requested RPC version not supported, min version=%d, max version=%d\n", mismatch_info->low, mismatch_info->high);
                return 14;

            case RPC__REJECT_STAT__AUTH_ERROR:
                fprintf(stdout, "Rejected RPC Reply: authentication failed with AuthStat %d\n", rejected_reply->auth_stat);
                return 15;
            // no other RejectStat exists
        }
    }    

    // now the reply must be AcceptedReply
    Rpc__AcceptedReply *accepted_reply = (rpc_reply->rbody)->areply;

    switch(accepted_reply->stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            return 0;
        case RPC__ACCEPT_STAT__PROG_MISMATCH: 
            Rpc__MismatchInfo *mismatch_info = accepted_reply->mismatch_info;
            fprintf(stdout, "Accepted RPC Reply: requested RPC program version not supported, min version=%d, max version=%d\n", mismatch_info->low, mismatch_info->high);
            return 16;
        default:
            fprintf(stdout, "Accepted RPC Reply with AcceptStat: %d\n", accepted_reply->stat);
            return 17;
    }
}

/*
* Given a RPC message, first it checks it's a valid RPC reply using 'check_rpc_msg_is_valid_rpc_reply', 
* and returns the error code if it fails.
* Then it checks the validated Rpc reply is an AcceptedReply with SUCCESS AcceptStat - returns the error code 
* if not.
* Finally, if the rpc_reply is a valid (validate_rpc_reply) AcceptedReply with SUCCESS AcceptStat, it returns 0.
*/
int validate_rpc_message_from_server(Rpc__RpcMsg *rpc_reply) {
    int error_code = check_rpc_msg_is_valid_rpc_reply(rpc_reply);
    if(error_code > 0) {
        return error_code;
    }

    error_code = check_valid_rpc_reply_is_AcceptedReply_with_success_AcceptStat(rpc_reply);
    if(error_code > 0) {
        return error_code;
    }

    return 0;
}