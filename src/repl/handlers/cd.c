#include "handlers.h" 

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

/*
* If there is a directory that is currently mounted, prints out all directory entries in the current
* working directory.
*/
void handle_cd(char *directory_name) {
    if(!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return;
    }

    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = directory_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(rpc_connection_context, diropargs, diropres);
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

    if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to enter the specified directory with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return;
    }

    // check that the file client wants to change cwd to is actually a directory
    if(diropres->diropok->attributes->nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        printf("Error: Not a directory: %s\n", directory_name);

        return;
    }

    // update the filesystem DAG
    if(strcmp(directory_name, ".") == 0) {
        // do nothing, stay in the same cwd
        nfs__dir_op_res__free_unpacked(diropres, NULL);
        return;
    }
    
    if(strcmp(directory_name, "..") == 0) {
        if(cwd_node->is_root) {
            // stay in the same cwd, we have no parent directory
            nfs__dir_op_res__free_unpacked(diropres, NULL);
            return;
        }

        cwd_node = cwd_node->parent;
        return;
    }

    char *child_absolute_path = get_file_absolute_path(cwd_node->absolute_path, directory_name);

    NfsFh__NfsFileHandle *child_nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *child_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);

    Nfs__FHandle *child_fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(child_fhandle);
    child_fhandle->nfs_filehandle = child_nfs_filehandle_copy;

    DAGNode *child_node = create_dag_node(strdup(child_absolute_path), diropres->diropok->attributes->nfs_ftype->ftype, child_fhandle, 0);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    free(child_absolute_path);
    if(filesystem_dag_root == NULL) {
        free(child_nfs_filehandle_copy);
        free(child_fhandle);

        printf("Error: Failed to create a new filesystem DAG node\n");

        return;
    }

    int error_code = add_child(cwd_node, child_node);
    if(error_code > 0) {
        free(child_nfs_filehandle_copy);
        free(child_fhandle);

        printf("Error: Failed to add the new filesystem DAG node as a child of the parent directory\n");

        return;
    }

    cwd_node = child_node;
}