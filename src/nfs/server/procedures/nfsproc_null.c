#include "nfsproc.h"

/*
* Runs the NFSPROC_NULL procedure (0).
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "nfs/None");
}