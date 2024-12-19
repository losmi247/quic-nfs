#include "server.h"

#include "procedures/nfsproc.h"

/*
* Calls the appropriate procedure in the Nfs RPC program based on the procedure number.
*
* Also forwards a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and corresponds to a supported authentication
* flavor) before being passed here.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *call_nfs(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        return create_prog_mismatch_accepted_reply(NFS_VERSION_LOW, NFS_VERSION_HIGH);
    }

    switch(procedure_number) {
        case 0:
            return serve_nfs_procedure_0_do_nothing(credential, verifier, parameters);
        case 1:
            return serve_nfs_procedure_1_get_file_attributes(credential, verifier, parameters);
        case 2:
            return serve_nfs_procedure_2_set_file_attributes(credential, verifier, parameters);
        case 3:
            // procedure 3 (NFSPROC_ROOT) is obsolete
            break;
        case 4:
            return serve_nfs_procedure_4_look_up_file_name(credential, verifier, parameters);
        case 5:
            return serve_nfs_procedure_5_read_from_symbolic_link(credential, verifier, parameters);
        case 6:
            return serve_nfs_procedure_6_read_from_file(credential, verifier, parameters);
        case 7:
        case 8:
            return serve_nfs_procedure_8_write_to_file(credential, verifier, parameters);
        case 9:
            return serve_nfs_procedure_9_create_file(credential, verifier, parameters);
        case 10:
            return serve_nfs_procedure_10_remove_file(credential, verifier, parameters);
        case 11:
            return serve_nfs_procedure_11_rename_file(credential, verifier, parameters);
        case 12:
        case 13:
            return serve_nfs_procedure_13_create_symbolic_link(credential, verifier, parameters);
        case 14:
            return serve_nfs_procedure_14_create_directory(credential, verifier, parameters);
        case 15:
            return serve_nfs_procedure_15_remove_directory(credential, verifier, parameters);
        case 16:
            return serve_nfs_procedure_16_read_from_directory(credential, verifier, parameters);
        case 17:
            return serve_nfs_procedure_17_get_filesystem_attributes(credential, verifier, parameters);
        default:
    }

    // procedure not found
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__PROC_UNAVAIL);
}