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
* Sends the given ReplyBody back to the RPC client in a RpcMsg, given the opened socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_reply_body(int rpc_client_socket_fd, Rpc__ReplyBody *reply_body) {
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
    int error_code = send_rm_record(rpc_client_socket_fd, rpc_msg_buffer, rpc_msg_size);
    free(rpc_msg_buffer);
    if(error_code > 0) {
        return 1;
    }

    return 0;
}

/*
* Wraps the procedure results given in the buffer 'results_buffer' of size 'results_size' into an Any
* message, along with a type 'results_type' of the result (e.g. nfs/AttrStat).
*
* The user of this function takes the responsibility to free the Any, the OpaqueAuth, and the
* AcceptedReply itself, using the the 'free_accepted_reply' function.
*/
Rpc__AcceptedReply *wrap_procedure_results_in_successful_accepted_reply(size_t results_size, uint8_t *results_buffer, char *results_type) {
    Rpc__AcceptedReply *accepted_reply = malloc(sizeof(Rpc__AcceptedReply));
    rpc__accepted_reply__init(accepted_reply);

    // currently supported AUTH_NONE and AUTH_SYS can always just send a AUTH_NONE OpaqueAuth from server
    // TODO (QNFS-52): Implement AUTH_SHORT
    accepted_reply->verifier = create_auth_none_opaque_auth();

    accepted_reply->stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply->reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = results_type;
    results->value.data = results_buffer;
    results->value.len = results_size;

    accepted_reply->results = results;

    return accepted_reply;
}

/*
* Builds and returns an AcceptedReply with PROG_MISMATCH AcceptStat, specifying the given 'low' and 'high' as
* lowest and highest versions of the RPC program available.
*
* The user of this function takes the responsibility to free the MismatchInfo, the OpaqueAuth, and the
* AcceptedReply itself, using the 'free_accepted_reply' function.
*/
Rpc__AcceptedReply *create_prog_mismatch_accepted_reply(uint32_t low, uint32_t high) {
    Rpc__AcceptedReply *accepted_reply = malloc(sizeof(Rpc__AcceptedReply));
    rpc__accepted_reply__init(accepted_reply);

    // currently supported AUTH_NONE and AUTH_SYS can always just send a AUTH_NONE OpaqueAuth from server
    // TODO (QNFS-52): Implement AUTH_SHORT
    accepted_reply->verifier = create_auth_none_opaque_auth();

    accepted_reply->stat = RPC__ACCEPT_STAT__PROG_MISMATCH;
    accepted_reply->reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO;

    Rpc__MismatchInfo *mismatch_info = malloc(sizeof(Rpc__MismatchInfo));
    rpc__mismatch_info__init(mismatch_info);
    mismatch_info->low = 2;
    mismatch_info->high = 2;

    accepted_reply->mismatch_info = mismatch_info;

    return accepted_reply;
}

/*
* Builds and returns an AcceptedReply with the given 'default_case_accept_stat' AcceptStat.
*
* If the given AcceptStat is RPC__ACCEPT_STAT__SUCCESS or RPC__ACCEPT_STAT__PROG_MISMATCH (so not default case
* AcceptStat), NULL is returned.
*
* The user of this function takes the responsibility to free the Empty, the OpaqueAuth, and the
* AcceptedReply itself, using the 'free_accepted_reply' function.
*/
Rpc__AcceptedReply *create_default_case_accepted_reply(Rpc__AcceptStat default_case_accept_stat) {
    if(default_case_accept_stat == RPC__ACCEPT_STAT__SUCCESS || default_case_accept_stat == RPC__ACCEPT_STAT__PROG_MISMATCH) {
        return NULL;
    }

    Rpc__AcceptedReply *accepted_reply = malloc(sizeof(Rpc__AcceptedReply));
    rpc__accepted_reply__init(accepted_reply);

    // currently supported AUTH_NONE and AUTH_SYS can always just send a AUTH_NONE OpaqueAuth from server
    // TODO (QNFS-52): Implement AUTH_SHORT
    accepted_reply->verifier = create_auth_none_opaque_auth();

    accepted_reply->stat = default_case_accept_stat;
    accepted_reply->reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    accepted_reply->default_case = empty;
    
    return accepted_reply;
}

/*
* Builds and returns an AcceptedReply with GARBAGE_ARGS AcceptStat.
*
* The user of this function takes the responsibility to deallocate the received
* AcceptedReply, using the 'free_accepted_reply' function.
*/
Rpc__AcceptedReply *create_garbage_args_accepted_reply(void) {
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__GARBAGE_ARGS);
}

/*
* Builds and returns an AcceptedReply with SYSTEM_ERR AcceptStat.
*
* The user of this function takes the responsibility to deallocate the received
* AcceptedReply, using the 'free_accepted_reply' function.
*/
Rpc__AcceptedReply *create_system_error_accepted_reply(void) {
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__SYSTEM_ERR);
}

/*
* Frees up heap-allocated memory in an AcceptedReply.
*
* Does nothing if 'accepted_reply' is NULL.
*/
void free_accepted_reply(Rpc__AcceptedReply *accepted_reply) {
    if(accepted_reply == NULL) {
        return;
    }

    free_opaque_auth(accepted_reply->verifier);

    if(accepted_reply->stat == RPC__ACCEPT_STAT__SUCCESS) {
        // clean up procedure results
        Google__Protobuf__Any *results = accepted_reply->results;

        if(results == NULL) {
            return;
        }

        if(results->value.data != NULL) {
            // free the buffer containing packed procedure results inside the Any
            free(results->value.data);
        }
        free(results);
    }
    else if(accepted_reply->stat == RPC__ACCEPT_STAT__PROG_MISMATCH) {
        free(accepted_reply->mismatch_info);
    }
    else {
        // for all other AcceptStat's - PROG_UNAVAIL, GARBAGE_ARGS, SYSTEM_ERR, etc. we only need to free the Empty default_case
        free(accepted_reply->default_case);
    }

    free(accepted_reply);
}

/*
* Sends the given AcceptedReply back to the RPC client, given the opened socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_accepted_reply_message(int rpc_client_socket_fd, Rpc__AcceptedReply *accepted_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_ACCEPTED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_AREPLY;  // reply_case is not actually transfered over network
    reply_body.areply = accepted_reply;

    return send_rpc_reply_body(rpc_client_socket_fd, &reply_body);
}

/*
* Builds and returns a RejectedReply with RPC_MISMATCH RejectStat (requested RPC version not supported),
* specifying 'low' and 'high' as lowest and highest version of RPC available.
*
* The user of this function takes the responsibility to free the MismatchInfo and the
* RejectedReply itself, using the 'free_rejected_reply' function.
*/
Rpc__RejectedReply *create_rpc_mismatch_rejected_reply(uint32_t low, uint32_t high) {
    Rpc__RejectedReply *rejected_reply = malloc(sizeof(Rpc__RejectedReply));
    rpc__rejected_reply__init(rejected_reply);

    rejected_reply->stat = RPC__REJECT_STAT__RPC_MISMATCH;
    rejected_reply->reply_data_case = RPC__REJECTED_REPLY__REPLY_DATA_MISMATCH_INFO;

    Rpc__MismatchInfo *mismatch_info = malloc(sizeof(Rpc__MismatchInfo));
    rpc__mismatch_info__init(mismatch_info);
    mismatch_info->low = low;
    mismatch_info->high = high;

    rejected_reply->mismatch_info = mismatch_info;

    return rejected_reply;
}

/*
* Builds and returns a RejectedReply with AUTH_ERROR RejectStat (RPC authentication failed), with the
* given AuthStat 'stat'.
*
* The user of this function takes the responsibility to free the RejectedReply 
* using the 'free_rejected_reply' function.
*/
Rpc__RejectedReply *create_auth_error_rejected_reply(Rpc__AuthStat stat) {
    Rpc__RejectedReply *rejected_reply = malloc(sizeof(Rpc__RejectedReply));
    rpc__rejected_reply__init(rejected_reply);

    rejected_reply->stat = RPC__REJECT_STAT__AUTH_ERROR;
    rejected_reply->reply_data_case = RPC__REJECTED_REPLY__REPLY_DATA_AUTH_STAT;

    rejected_reply->auth_stat = stat;

    return rejected_reply;
}

/*
* Frees up heap-allocated memory in a RejectedReply.
*
* Does nothing if 'rejected_reply' is NULL.
*/
void free_rejected_reply(Rpc__RejectedReply *rejected_reply) {
    if(rejected_reply == NULL) {
        return;
    }

    if(rejected_reply->stat == RPC__REJECT_STAT__RPC_MISMATCH) {
        free(rejected_reply->mismatch_info);
    }

    free(rejected_reply);
}

/*
* Sends the given RejectedReply back to the RPC client, given the opened socket for that client.
*
* Returns 0 on success, and > 0 on failure.
*/
int send_rpc_rejected_reply_message(int rpc_client_socket_fd, Rpc__RejectedReply *rejected_reply) {
    Rpc__ReplyBody reply_body = RPC__REPLY_BODY__INIT;
    reply_body.stat = RPC__REPLY_STAT__MSG_DENIED;
    reply_body.reply_case = RPC__REPLY_BODY__REPLY_RREPLY;  // reply_case is not actually transfered over network
    reply_body.rreply = rejected_reply;

    return send_rpc_reply_body(rpc_client_socket_fd, &reply_body);
}

/*
* Prints out the given error message, and sends an AUTH_ERROR RejectedReply with the given AuthStat to the 
* given opened client socket.
*
* Returns 0 on success and > 0 on failure.
*/
int send_auth_error_rejected_reply(int rpc_client_socket_fd, char *error_msg, Rpc__AuthStat auth_stat) {
    fprintf(stdout, "%s", error_msg);

    Rpc__RejectedReply *rejected_reply = create_auth_error_rejected_reply(auth_stat);

    int error_code = send_rpc_rejected_reply_message(rpc_client_socket_fd, rejected_reply);
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
* and sends an appropriate RejectedReply to the given opened 'rpc_client_socket_fd'.
*/
int validate_credential_and_verifier(int rpc_client_socket_fd, Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier) {
    int error_code;

    if(credential == NULL) {
        error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with 'credential' being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(credential->empty == NULL) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE credential, with credential->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        if(credential->body_case != RPC__OPAQUE_AUTH__BODY_AUTH_SYS) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with inconsistent credential->flavor and credential->body_case.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }

        if(credential->auth_sys == NULL) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        Rpc__AuthSysParams *authsysparams = credential->auth_sys;

        if(authsysparams->machinename == NULL) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->machinename being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
        if(authsysparams->gids == NULL) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_SYS credential, with credential->auth_sys->gids being NULL.\n", RPC__AUTH_STAT__AUTH_BADCRED);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // TODO (QNFS-52): Implement AUTH_SHORT
        error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with unsupported authentication flavor %d.\n", RPC__AUTH_STAT__AUTH_BADCRED);
        return error_code > 0 ? -1 : 1;
    }

    if(verifier == NULL) {
        error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with 'verifier' being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }
    if(verifier->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(verifier->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE verifier, with inconsistent verifier->flavor and verifier->body_case.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
        if(verifier->empty == NULL) {
            error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with AUTH_NONE verifier, with verifier->empty being NULL.\n", RPC__AUTH_STAT__AUTH_BADVERF);
            return error_code > 0 ? -1 : 1;
        }
    }
    else {
        // in AUTH_NONE, AUTH_SYS, and AUTH_SHORT, verifier in CallBody always has AUTH_NONE flavor
        error_code = send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with 'verifier' having unsupported flavor.\n", RPC__AUTH_STAT__AUTH_BADVERF);
        return error_code > 0 ? -1 : 1;
    }

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

    // check RPC version
    if(call_body->rpcvers != 2) {
        fprintf(stdout, "Server received an RPC call with RPC version not equal to 2.\n");

        Rpc__RejectedReply *rejected_reply = create_rpc_mismatch_rejected_reply(2, 2);
    
        int error_code = send_rpc_rejected_reply_message(rpc_client_socket_fd, rejected_reply);
        free_rejected_reply(rejected_reply);
        if(error_code > 0) {
            fprintf(stdout, "Server failed to send RPC mismatch RejectedReply\n");
            return 5;
        }

        return 0;
    }

    // check authentication fields
    int error_code = validate_credential_and_verifier(rpc_client_socket_fd, call_body->credential, call_body->verifier);
    if(error_code != 0) {
        return 6;
    }
    log_rpc_call_body_info(call_body);
    if(call_body->credential->flavor == RPC__AUTH_FLAVOR__AUTH_NONE && call_body->proc != 0) {
        // only NULL procedure is allowed to use AUTH_NONE flavor
        return send_auth_error_rejected_reply(rpc_client_socket_fd, "Server received an RPC call with authentication flavor AUTH_NONE for a non-NULL procedure.\n", RPC__AUTH_STAT__AUTH_TOOWEAK);
    }

    // reply with a AcceptedReply
    Google__Protobuf__Any *parameters = call_body->params;

    Rpc__AcceptedReply *accepted_reply = forward_rpc_call_to_program(call_body->credential, call_body->verifier, call_body->prog, call_body->vers, call_body->proc, parameters);
    rpc__rpc_msg__free_unpacked(rpc_call, NULL);

    error_code = send_rpc_accepted_reply_message(rpc_client_socket_fd, accepted_reply);
    free_accepted_reply(accepted_reply);
    if(error_code > 0) {
        fprintf(stdout, "Server failed to send AcceptedReply\n");
        return 7;
    }

    return 0;
}