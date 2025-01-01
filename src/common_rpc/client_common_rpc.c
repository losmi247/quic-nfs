#include "client_common_rpc.h"

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

    if(accepted_reply->verifier == NULL) {
        fprintf(stderr, "Something went wrong at server - NULL verifier in received RPC reply.\n");
        return 10;
    }
    Rpc__OpaqueAuth *verifier = accepted_reply->verifier;
    if(verifier->flavor == RPC__AUTH_FLAVOR__AUTH_NONE) {
        if(verifier->body_case != RPC__OPAQUE_AUTH__BODY_EMPTY) {
            fprintf(stderr, "Something went wrong at server - inconsistent OpaqueAuth->flavor and body_case.\n");
            return 11;
        }
        if(verifier->empty == NULL) {
            fprintf(stderr, "Something went wrong at server - OpaqueAuth flavor is AUTH_NONE, but NULL OpaqueAuth->empty, in received RPC reply.\n");
            return 12;
        }
    }
    // TODO (QNFS-52): Implement AUTH_SHORT
    else {
        fprintf(stderr, "Something went wrong at server - verifier flavor is %d which is not supported, in received RPC reply.\n", verifier->flavor);
        return 13;
    }

    switch(accepted_reply->stat) {
        case RPC__ACCEPT_STAT__SUCCESS:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 14;
            }

            Google__Protobuf__Any *procedure_results = accepted_reply->results;
            if(procedure_results == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL results in received RPC reply.\n");
                return 15;
            }

            return 0;
        case RPC__ACCEPT_STAT__PROG_MISMATCH:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 16;
            }

            Rpc__MismatchInfo *mismatch_info = accepted_reply->mismatch_info;
            if(mismatch_info == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL mismatch_info in received RPC reply.\n");
                return 17;
            }

            return 0;
        default:
            if(accepted_reply->reply_data_case != RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE) {
                fprintf(stderr, "Something went wrong at server - inconsistent AcceptStat and reply_data_case in received RPC reply.\n");
                return 18;
            }

            if(accepted_reply->default_case == NULL) {
                fprintf(stderr, "Something went wrong at server - NULL default_case in received RPC reply.\n");
                return 19;
            }
            return 0;
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
                fprintf(stdout, "Rejected RPC Reply: authentication failed with AuthStat %s\n", auth_stat_to_string(rejected_reply->auth_stat));
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
            char *accept_stat = rpc_accept_stat_to_string(accepted_reply->stat);
            fprintf(stderr, "Accepted RPC Reply with AcceptStat: %s\n", accept_stat);
            free(accept_stat);
            return 5;
    }
}