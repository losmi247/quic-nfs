#include "handlers.h" 

#include "src/path_building/path_building.h"

/*
* Removes a file with the given name from the current working directory.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_rm(char *file_name) {
    if(!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return 1;
    }

    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = file_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;
    diropargs.name = &filename;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_10_remove_file(rpc_connection_context, diropargs, nfsstat);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(nfsstat);

        return 1;
    }

    if(validate_nfs_nfs_stat(nfsstat) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        return 1;
    }

    if(nfsstat->stat == NFS__STAT__NFSERR_ACCES) {
        printf("rm: Permission denied\n");
        
        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        return 1;
    }
    else if(nfsstat->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(nfsstat->stat);
        printf("Error: Failed to remove a file in the current working directory with status %s\n", string_status);
        free(string_status);

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        return 1;
    }

    return 0;
}