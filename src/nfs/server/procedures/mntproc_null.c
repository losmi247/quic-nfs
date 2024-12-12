#include "mntproc.h"

/*
* Runs the MOUNTPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_mnt_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "mount/None");
}