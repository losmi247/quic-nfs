#include "handlers.h"

#include "src/path_building/path_building.h"

/*
 * Enters the given directory inside the current working directory.
 *
 * Returns 0 on success and > 0 on failure.
 */
int handle_cd(char *directory_name) {
    if (!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return 1;
    }

    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = directory_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(rpc_connection_context, diropargs, diropres);
    if (status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(diropres);

        return 1;
    }

    if (validate_nfs_dir_op_res(diropres) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    if (diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("cd: Permission denied: %s\n", directory_name);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    } else if (diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to enter the specified directory with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    // check that the file client wants to change cwd to is actually a directory
    if (diropres->diropok->attributes->nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        printf("Error: Not a directory: %s\n", directory_name);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    // update the filesystem DAG
    if (strcmp(directory_name, ".") == 0) {
        // do nothing, stay in the same cwd
        nfs__dir_op_res__free_unpacked(diropres, NULL);
        return 0;
    }

    if (strcmp(directory_name, "..") == 0) {
        if (cwd_node->is_root) {
            // stay in the same cwd, we have no parent directory
            nfs__dir_op_res__free_unpacked(diropres, NULL);
            return 0;
        }

        cwd_node = cwd_node->parent;
        return 0;
    }

    char *child_absolute_path = get_file_absolute_path(cwd_node->absolute_path, directory_name);

    // check that client has execute access to this child directory
    status = check_execute_permission(child_absolute_path, rpc_connection_context->credential->auth_sys->uid,
                                      rpc_connection_context->credential->auth_sys->gid);
    if (status < 0) {
        printf("Error: Failed to check if client has execute permission on directory %s with status %d\n",
               child_absolute_path, status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);
        free(child_absolute_path);

        return 1;
    }
    if (status == 1) {
        printf("cd: Permission denied: %s\n", directory_name);

        nfs__dir_op_res__free_unpacked(diropres, NULL);
        free(child_absolute_path);

        return 1;
    }

    NfsFh__NfsFileHandle *child_nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *child_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);

    Nfs__FHandle *child_fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(child_fhandle);
    child_fhandle->nfs_filehandle = child_nfs_filehandle_copy;

    DAGNode *child_node =
        create_dag_node(child_absolute_path, diropres->diropok->attributes->nfs_ftype->ftype, child_fhandle, 0);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    free(child_absolute_path);
    if (child_node == NULL) {
        printf("Error: Failed to create a new filesystem DAG node\n");

        free(child_nfs_filehandle_copy);
        free(child_fhandle);

        return 1;
    }

    int error_code = add_child(cwd_node, child_node);
    if (error_code > 0) {
        printf("Error: Failed to add the new filesystem DAG node as a child of the parent directory\n");

        free(child_nfs_filehandle_copy);
        free(child_fhandle);

        return 1;
    }

    cwd_node = child_node;
}