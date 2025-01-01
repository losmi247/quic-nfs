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

#include "src/transport/tcp/tcp_record_marking.h"

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
