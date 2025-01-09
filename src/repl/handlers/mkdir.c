#include "handlers.h" 

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

/*
* Creates a directory with the given name inside the current working directory.
*/
void handle_mkdir(char *directory_name) {
    if(!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return;
    }

    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = directory_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;;
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
        free(diropres);

        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        return;
    }

    if(validate_nfs_dir_op_res(diropres) > 0) {
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        printf("Error: Invalid NFS procedure result received from the server\n");

        return;
    }

    if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("mkdir: Permission denied\n");
        
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return;
    }
    else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to create a directory in the current working directory with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return;
    }

    // create a new filesystem DAG node for the created directory
    char *child_directory_absolute_path = get_file_absolute_path(cwd_node->absolute_path, directory_name);

    NfsFh__NfsFileHandle *child_directory_nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *child_directory_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);

    Nfs__FHandle *child_directory_fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(child_directory_fhandle);
    child_directory_fhandle->nfs_filehandle = child_directory_nfs_filehandle_copy;

    DAGNode *child_node = create_dag_node(child_directory_absolute_path, diropres->diropok->attributes->nfs_ftype->ftype, child_directory_fhandle, 0);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    free(child_directory_absolute_path);
    if(child_node == NULL) {
        free(child_directory_nfs_filehandle_copy);
        free(child_directory_fhandle);

        printf("Error: Failed to create a new filesystem DAG node\n");

        return;
    }

    int error_code = add_child(cwd_node, child_node);
    if(error_code > 0) {
        free(child_directory_nfs_filehandle_copy);
        free(child_directory_fhandle);

        printf("Error: Failed to add the new filesystem DAG node as a child of the parent directory\n");

        return;
    }
}