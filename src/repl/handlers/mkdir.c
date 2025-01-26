#include "handlers.h" 

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

/*
* Creates a directory with the given name inside the current working directory.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_mkdir(char *directory_name) {
    if(!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return 1;
    }

    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = directory_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;
    diropargs.name = &filename;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = -1;
    sattr.uid = rpc_connection_context->credential->auth_sys->uid;  // the REPL that creates the file is the owner of the file
    sattr.gid = rpc_connection_context->credential->auth_sys->gid;
    sattr.size = -1;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = -1;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;
    createargs.attributes = &sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_14_create_directory(rpc_connection_context, createargs, diropres);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(diropres);

        return 1;
    }

    if(validate_nfs_dir_op_res(diropres) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("mkdir: Permission denied\n");
        
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }
    else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to create a directory in the current working directory with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    return 0;
}