#include "nfsproc.h"

/*
 * Runs the NFSPROC_NULL procedure (0).
 *
 * Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
 * credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported
 * authentication flavor) before being passed here. This procedure can be given an AUTH_NONE credential+verifier pair.
 *
 * The user of this function takes the responsibility to deallocate the received AcceptedReply
 * using the 'free_accepted_reply()' function.
 */
Rpc__AcceptedReply *serve_nfs_procedure_0_do_nothing(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier,
                                                     Google__Protobuf__Any *parameters) {
    // NULL procedure accepts AUTH_NONE authentication flavor

    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "nfs/None");
}