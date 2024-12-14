#include "handlers.h" 

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

/*
* If there is a directory that is currently mounted, prints out all directory entries in the current
* working directory.
*/
void handle_cd(char *directory_name) {
    if(cwd_absolute_path == NULL) {
        printf("Error: No remote file system is currently mounted\n");
        return;
    }

    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = directory_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_filehandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(server_ipv4_addr, server_port_number, diropargs, diropres);
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
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to enter the specified directory with status %s\n", string_status);
        free(string_status);

        return;
    }

    // check that the file client wants to change cwd to is actually a directory
    if(diropres->diropok->attributes->nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        printf("Error: Not a a directory: %s\n", directory_name);

        return;
    }

    // update the Nfs client state
    char *new_cwd_absolute_path = get_file_absolute_path(cwd_absolute_path, directory_name);
    free(cwd_absolute_path);
    cwd_absolute_path = new_cwd_absolute_path;

    NfsFh__NfsFileHandle *nfs_filehandle_copy = malloc(sizeof(NfsFh__NfsFileHandle));
    *nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    free(cwd_filehandle->nfs_filehandle);
    cwd_filehandle->nfs_filehandle = nfs_filehandle_copy;
}