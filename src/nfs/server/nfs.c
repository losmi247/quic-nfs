#include "server.h"

#include "procedures/nfsproc.h"

/*
* Calls the appropriate procedure in the Nfs RPC program based on the procedure number.
*/
Rpc__AcceptedReply *call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        return create_prog_mismatch_accepted_reply(NFS_VERSION_LOW, NFS_VERSION_HIGH);
    }

    switch(procedure_number) {
        case 0:
            return serve_nfs_procedure_0_do_nothing(parameters);
        case 1:
            return serve_nfs_procedure_1_get_file_attributes(parameters);
        case 2:
            return serve_nfs_procedure_2_set_file_attributes(parameters);
        case 3:
            // procedure 3 (NFSPROC_ROOT) is obsolete
            break;
        case 4:
            return serve_nfs_procedure_4_look_up_file_name(parameters);
        case 5:
        case 6:
            return serve_nfs_procedure_6_read_from_file(parameters);
        case 7:
        case 8:
            return serve_nfs_procedure_8_write_to_file(parameters);
        case 9:
            return serve_nfs_procedure_9_create_file(parameters);
        case 10:
            return serve_nfs_procedure_10_remove_file(parameters);
        case 11:
            return serve_nfs_procedure_11_rename_file(parameters);
        case 12:
        case 13:
            return serve_nfs_procedure_13_create_symbolic_link(parameters);
        case 14:
            return serve_nfs_procedure_14_create_directory(parameters);
        case 15:
            return serve_nfs_procedure_15_remove_directory(parameters);
        case 16:
            return serve_nfs_procedure_16_read_from_directory(parameters);
        case 17:
            return serve_nfs_procedure_17_get_filesystem_attributes(parameters);
        default:
    }

    // procedure not found
    return create_default_case_accepted_reply(RPC__ACCEPT_STAT__PROC_UNAVAIL);
}