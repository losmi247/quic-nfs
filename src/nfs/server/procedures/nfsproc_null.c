#include "nfsproc.h"

/*
* Runs the NFSPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_nfs_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "nfs/None");
}