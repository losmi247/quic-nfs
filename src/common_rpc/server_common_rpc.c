#include <stdio.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"

#include "common_rpc.h"
#include "server_common_rpc.h"

#include "record_marking.h"

/*
* Wraps the procedure results given in the buffer 'results_buffer' of size 'results_size' into an Any
* message, along with a type 'results_type' of the result (e.g. nfs/AttrStat).
*
* The user of this function takes the responsibility to free the Any and OpaqueAuth 
* allocated here (this is done by the 'clean_up_accepted_reply' function after the RPC is sent).
*/
Rpc__AcceptedReply wrap_procedure_results_in_successful_accepted_reply(size_t results_size, uint8_t *results_buffer, char *results_type) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.verifier = create_auth_none_opaque_auth();
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = results_type;
    results->value.data = results_buffer;
    results->value.len = results_size;

    accepted_reply.results = results;

    return accepted_reply;
}

/*
* Builds and returns an AcceptedReply with GARBAGE_ARGS AcceptStat.
*
* The user of this function takes the responsibility to free the Empty and OpaqueAuth 
* allocated here (this is done by the 'clean_up_accepted_reply' function after the RPC is sent).
*/
Rpc__AcceptedReply create_garbage_args_accepted_reply(void) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.verifier = create_auth_none_opaque_auth();
    accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    accepted_reply.default_case = empty;
    
    return accepted_reply;
}

/*
* Builds and returns an AcceptedReply with SYSTEM_ERR AcceptStat.
*
* The user of this function takes the responsibility to free the Empty and OpaqueAuth 
* allocated here (this is done by the 'clean_up_accepted_reply' function after the RPC is sent).
*/
Rpc__AcceptedReply create_system_error_accepted_reply(void) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.verifier = create_auth_none_opaque_auth();
    accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    accepted_reply.default_case = empty;
    
    return accepted_reply;
}

/*
* Frees up heap-allocated memory for procedure results or MismatchInfo in an AcceptedReply.
*/
void clean_up_accepted_reply(Rpc__AcceptedReply accepted_reply) {
    free_opaque_auth(accepted_reply.verifier);

    if(accepted_reply.stat == RPC__ACCEPT_STAT__SUCCESS) {
        // clean up procedure results
        Google__Protobuf__Any *results = accepted_reply.results;

        if(results == NULL) {
            return;
        }

        if(results->value.data != NULL) {
            // free the buffer containing packed procedure results inside the Any
            free(results->value.data);
        }
        free(results);

        return;
    }

    if(accepted_reply.stat == RPC__ACCEPT_STAT__PROG_MISMATCH) {
        free(accepted_reply.mismatch_info);
        return;
    }

    // for all other AcceptStat's - PROG_UNAVAIL, GARBAGE_ARGS, SYSTEM_ERR, etc. we only need to free the Empty default_case
    free(accepted_reply.default_case);
}

/*
* Sends an accepted RPC Reply back to the RPC client, given the opened socket for that client.
* Returns 0 on success, and > 0 on failure.
*
* Heap-allocated state for procedure results in accepted_reply is freed here after the RPC reply is sent to 
* client using the clean_up_accepted_reply().
*/
int send_rpc_accepted_reply_message(int rpc_client_socket_fd, Rpc__AcceptedReply accepted_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_ACCEPTED;
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

    // send serialized RpcMsg back to the client as a single Record Marking record
    int error_code = send_rm_record(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    if(error_code > 0) {
        free(rpc_msg_buffer);
        clean_up_accepted_reply(accepted_reply);

        return 1;
    }

    free(rpc_msg_buffer);
    clean_up_accepted_reply(accepted_reply);

    return 0;
}

/*
* Sends a rejected RPC Reply back to the RPC client, given the opened socket for that client.
* Returns 0 on success, and > 0 on failure.
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

    // send serialized RpcMsg back to the client as a single Record Marking record
    int error_code = send_rm_record(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    if(error_code > 0) {
        free(rpc_msg_buffer);
        return 1;
    }

    free(rpc_msg_buffer);

    return 0;
}

/*
* Takes an opened client socket and reads and processes one RPC from it.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_client(int rpc_client_socket_fd) {
    // read one RPC call as a single Record Marking record
    size_t rpc_msg_size = -1;
    uint8_t *rpc_msg_buffer = receive_rm_record(rpc_client_socket_fd, &rpc_msg_size);
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
    log_rpc_call_body_info(call_body);

    // now we have a valid RPC, so reply to it
    if(call_body->rpcvers != 2) {
        fprintf(stdout, "Server received an RPC call with RPC version not equal to 2.\n");

        Rpc__MismatchInfo mismatch_info = RPC__MISMATCH_INFO__INIT;
        mismatch_info.low = 2;
        mismatch_info.high = 2;
        send_rpc_rejected_reply_message(rpc_client_socket_fd, RPC__REPLY_STAT__MSG_DENIED, RPC__REJECT_STAT__RPC_MISMATCH, &mismatch_info, NULL);

        return 0;
    }

    Google__Protobuf__Any *parameters = call_body->params;

    Rpc__AcceptedReply accepted_reply = forward_rpc_call_to_program(call_body->prog, call_body->vers, call_body->proc, parameters);
    send_rpc_accepted_reply_message(rpc_client_socket_fd, accepted_reply);
    
    rpc__rpc_msg__free_unpacked(rpc_call, NULL);

    return 0;
}