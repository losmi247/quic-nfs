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
* Sends an RPC call for the given program number, program version, procedure number, and parameters,
* to the given opened socket.
*
* Returns the RPC reply received from the server on success, and NULL on failure.
*
* The user of this function takes on the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *execute_rpc_call(int rpc_client_socket_fd, Rpc__RpcMsg *call_rpc_msg) {
    if(call_rpc_msg == NULL) {
        return NULL;
    }

    // serialize RpcMsg
    size_t rpc_msg_size = rpc__rpc_msg__get_packed_size(call_rpc_msg);
    uint8_t *rpc_msg_buffer = malloc(sizeof(uint8_t) * rpc_msg_size);
    rpc__rpc_msg__pack(call_rpc_msg, rpc_msg_buffer);

    // send the serialized RpcMsg to the server as a single Record Marking record
    // TODO: (QNFS-37) implement time-outs + reconnections
    int error_code = send_rm_record(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    if(error_code > 0) {
        return NULL;
    }

    // receive the RPC reply from the server as a single Record Marking record
    size_t reply_rpc_msg_size = -1;
    uint8_t *reply_rpc_msg_buffer = receive_rm_record(rpc_client_socket_fd, &reply_rpc_msg_size);
    if(reply_rpc_msg_buffer == NULL) {
        return NULL;
    }

    Rpc__RpcMsg *reply_rpc_msg = deserialize_rpc_msg(reply_rpc_msg_buffer, reply_rpc_msg_size);
    free(reply_rpc_msg_buffer);

    return reply_rpc_msg;
}

/*
* Given the RPC program number to be called, program version, procedure number, and the parameters for it, calls
* the appropriate remote procedure.
*
* Returns the server's RPC reply on success, and NULL on failure.
*
* The user of this function takes the responsibility to call 'rpc__rpc_msg__free_unpacked(rpc_reply, NULL)' 
* when it's done using the rpc_reply and it's subfields (e.g. procedure parameters).
*/
Rpc__RpcMsg *invoke_rpc_remote(const char *server_ipv4_addr, uint16_t server_port, uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any parameters) {
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

    Rpc__CallBody call_body = RPC__CALL_BODY__INIT;
    call_body.rpcvers = 2;
    call_body.prog = program_number;
    call_body.vers = program_version;
    call_body.proc = procedure_number;
    call_body.params = &parameters;

    Rpc__RpcMsg call_rpc_msg = RPC__RPC_MSG__INIT;
    call_rpc_msg.xid = generate_rpc_xid();
    call_rpc_msg.mtype = RPC__MSG_TYPE__CALL;
    call_rpc_msg.body_case = RPC__RPC_MSG__BODY_CBODY; // this body_case enum is not actually sent over the network
    call_rpc_msg.cbody = &call_body;

    Rpc__RpcMsg *reply_rpc_msg = execute_rpc_call(rpc_client_socket_fd, &call_rpc_msg);
    close(rpc_client_socket_fd);

    return reply_rpc_msg;
}

/*
* Checks that the given RPC message is a valid RPC reply.
* 
* If the RPC message is a valid RPC reply, 0 is returned.
* If the RPC message has incosistencies, the errors are printed to stderr and error code > 0 is returned.
*/
int validate_rpc_reply_structure(Rpc__RpcMsg *rpc_reply) {
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
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 14;
            }

            if(accepted_reply->default_case == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL default_case in received RPC reply.\n");
                return 15;
            }
            return 0;
    }
}

char *rpc_accept_stat_to_string(Rpc__AcceptStat accept_stat) {
    switch(accept_stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            return strdup("SUCCESS");
        case RPC__ACCEPT_STAT__PROG_UNAVAIL:
            return strdup("SUCCESS");
        case RPC__ACCEPT_STAT__PROG_MISMATCH:
            return strdup("SUCCESS");
        case RPC__ACCEPT_STAT__PROC_UNAVAIL:
            return strdup("SUCCESS");
        case RPC__ACCEPT_STAT__GARBAGE_ARGS:
            return strdup("SUCCESS");
        case RPC__ACCEPT_STAT__SYSTEM_ERR:
            return strdup("SUCCESS");
        default:
            return strdup("Unknown");
    }
}

/*
* Given a RPC message, first validates its structure using 'validate_rpc_reply_structure' to ensure it is a
* valid RPC reply. Then checks that this RPC reply is has AcceptedReply as body and that the AcceptedReply within
* it has SUCCESS AcceptStat.
*
* Returns 0 if the RPC message is a valid RPC reply with AcceptedReply as body and SUCCESS AcceptStat, otherwise an
* appropriate error message is printed and an error code > 0 is returned.
*/
int validate_successful_accepted_reply(Rpc__RpcMsg *rpc_reply) {
    int error_code = validate_rpc_reply_structure(rpc_reply);
    if(error_code > 0) {
        return 1;
    }
    
    Rpc__ReplyBody *reply_body = rpc_reply->rbody;
    if(reply_body->stat == RPC__REPLY_STAT__MSG_DENIED) {
        Rpc__RejectedReply *rejected_reply = reply_body->rreply;

        switch(rejected_reply->stat) {
            case RPC__REJECT_STAT__RPC_MISMATCH:
                Rpc__MismatchInfo *mismatch_info = rejected_reply->mismatch_info;
                fprintf(stderr, "Rejected RPC Reply: requested RPC version not supported, min version=%d, max version=%d\n", mismatch_info->low, mismatch_info->high);
                return 2;

            case RPC__REJECT_STAT__AUTH_ERROR:
                fprintf(stdout, "Rejected RPC Reply: authentication failed with AuthStat %d\n", rejected_reply->auth_stat);
                return 3;
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
            fprintf(stderr, "Accepted RPC Reply: requested RPC program version not supported, min version=%d, max version=%d\n", mismatch_info->low, mismatch_info->high);
            return 4;
        default:
            fprintf(stderr, "Accepted RPC Reply with AcceptStat: %s\n", rpc_accept_stat_to_string(accepted_reply->stat));
            return 5;
    }
}