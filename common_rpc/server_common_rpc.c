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
#include "server_common_rpc.h"

/*
* Sends an accepted RPC Reply back to the RPC client.
* Returns 0 on successful send of a reply, and > 0 on error.
*
* All heap-allocated state in accepted_reply is freed here after the RPC reply is sent to client (e.g. mismatch_info, procedure results)
* using the clean_up_accepted_reply() which is implemented by the specific RPC program (because different RPC programs will have
* different ways of freeing procedure results).
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

    // send serialized RpcMsg back to the client over network
    write(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);

    free(rpc_msg_buffer);

    clean_up_accepted_reply(accepted_reply);

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

/*
* Takes the socket of the client which sent the RPC call and processes it.
*/
void handle_client(int rpc_client_socket_fd) {
    uint8_t *rpc_msg_buffer = (uint8_t *) malloc(RPC_MSG_BUFFER_SIZE * sizeof(uint8_t));
    size_t bytes_received = recv(rpc_client_socket_fd, rpc_msg_buffer, RPC_MSG_BUFFER_SIZE, 0);

    Rpc__RpcMsg *rpc_call = deserialize_rpc_msg(rpc_msg_buffer, bytes_received);
    free(rpc_msg_buffer);
    if(rpc_call == NULL) {
        // here send a RPC reply with appropriate status code saying what went wrong
        return;
    }

    // receiving a RPC reply at server should just be ignored
    if(rpc_call->mtype != RPC__MSG_TYPE__CALL || rpc_call->body_case != RPC__RPC_MSG__BODY_CBODY) {
        fprintf(stderr, "Server received an RPC reply but it should only be receiving RPC calls.");

        return;
    }

    Rpc__CallBody *call_body = rpc_call->cbody;
    if(call_body == NULL) {
        // here send a RPC reply with appropriate status code saying what went wrong (rejected_reply)
        return;
    }
    if(call_body->rpcvers != 2) {
        fprintf(stderr, "Server received an RPC call with RPC version not equal to 2.");

        Rpc__MismatchInfo mismatch_info = RPC__MISMATCH_INFO__INIT;
        mismatch_info.low = 2;
        mismatch_info.high = 2;
        send_rpc_rejected_reply_message(rpc_client_socket_fd, RPC__REPLY_STAT__MSG_DENIED, RPC__REJECT_STAT__RPC_MISMATCH, &mismatch_info, NULL);

        return;
    }
    Google__Protobuf__Any *parameters = call_body->params;

    Rpc__AcceptedReply accepted_reply = forward_rpc_call_to_program(call_body->prog, call_body->vers, call_body->proc, parameters);
    send_rpc_accepted_reply_message(rpc_client_socket_fd, accepted_reply);
    
    rpc__rpc_msg__free_unpacked(rpc_call, NULL);
}