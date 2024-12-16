#include "server.h"

#include "procedures/mntproc.h"

/*
* Calls the appropriate procedure in the Mount RPC program based on the procedure number.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        return create_prog_mismatch_accepted_reply(NFS_VERSION_LOW, NFS_VERSION_HIGH);
    }

    switch(procedure_number) {
        case 0:
            return serve_mnt_procedure_0_do_nothing(parameters);
        case 1:
            return serve_mnt_procedure_1_add_mount_entry(parameters);
        case 2:
        case 3:
        case 4:
        case 5:
        default:
    }

    // procedure not found
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__PROC_UNAVAIL);
}