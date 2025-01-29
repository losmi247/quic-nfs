#include "path_resolution.h"

#include "src/filehandle_management/filehandle_management.h"
#include "src/message_validation/message_validation.h"
#include "src/parsing/parsing.h"

#include "src/nfs/clients/nfs_client.h"

/*
* Given the PRC connection context and the fhandle of the filesystem root, resolves the given absolute path
* (i.e. starting with a /) to a file as specified in https://man7.org/linux/man-pages/man7/path_resolution.7.html.
*
* Performs LOOKUP procedures from the filesystem root to resolve component by component of the
* pathname. Returns the NFS fhandle of the resolved file and places that file's type in the 'ftype' argument.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the received Nfs__FHandle and
* the NfsFh__NfsFileHandle inside it.
*/
Nfs__FHandle *resolve_absolute_path(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *filesystem_root_fhandle, char *path, Nfs__FType *ftype) {    
    if(rpc_connection_context == NULL) {
        return NULL;
    }

    if(filesystem_root_fhandle == NULL) {
        return NULL;
    }

    if(path == NULL) {
        return NULL;
    }

    if(path[0] != '/') {  // we only resolve absolute paths
        return NULL;
    }

    Nfs__FHandle *curr_fhandle = filesystem_root_fhandle;
    Nfs__FType curr_ftype = NFS__FTYPE__NFDIR;

    int pathname_length = strlen(path);
    char *path_copy = malloc(sizeof(char) *  (pathname_length + 1));
    strncpy(path_copy, path, pathname_length);
    path_copy[pathname_length] = '\0';

    // look up component by component of the pathname
    char *save_ptr;
    char *pathname_component = strtok_r(path_copy, "/", &save_ptr);
    int is_first_component = 1;
    while(pathname_component != NULL) {
        if(curr_ftype != NFS__FTYPE__NFDIR) {
            free(path_copy);
            return NULL;
        }

        // look up this pathname component
        Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
        diropargs.dir = curr_fhandle;
        Nfs__FileName file_name = NFS__FILE_NAME__INIT;
        file_name.filename = pathname_component;
        diropargs.name = &file_name;

        Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
        int status = nfs_procedure_4_look_up_file_name(rpc_connection_context, diropargs, diropres);
        if(status != 0) {
            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            free(path_copy);
            free(diropres);

            return NULL;
        }

        if(validate_nfs_dir_op_res(diropres) > 0) {
            printf("Error: Invalid NFS LOOKUP procedure result received from the server\n");

            free(path_copy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }

        if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("Error: Permission denied\n");
            
            free(path_copy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }
        else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
            printf("Error: Failed to lookup pathname component %s while resolving pathname %s, with status %s\n", pathname_component, path, string_status);
            free(string_status);

            free(path_copy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }
                
        curr_ftype = diropres->diropok->attributes->nfs_ftype->ftype;

        // free the previous file handle if it's not the starting one
        if(!is_first_component) {
            free(curr_fhandle->nfs_filehandle);
            free(curr_fhandle);
        }
        is_first_component = 0;

        // make a stack allocated fhandle for this looked up file
        NfsFh__NfsFileHandle *nfs_filehandle = malloc(sizeof(NfsFh__NfsFileHandle));
        if(nfs_filehandle == NULL) {
            return NULL;
        }
        *nfs_filehandle = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
        nfs__dir_op_res__free_unpacked(diropres, NULL);
        curr_fhandle = malloc(sizeof(Nfs__FHandle));
        nfs__fhandle__init(curr_fhandle);
        if(curr_fhandle == NULL) {
            return NULL;
        }
        curr_fhandle->nfs_filehandle = nfs_filehandle;

        // get the next pathname component
        pathname_component = strtok_r(NULL, "/", &save_ptr);
    }

    free(path_copy);

    *ftype = curr_ftype;

    // make a heap allocated copy of the resolved file's NFS fhandle
    Nfs__FHandle *file_fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(file_fhandle);
    NfsFh__NfsFileHandle *file_nfs_filehandle = malloc(sizeof(NfsFh__NfsFileHandle));
    nfs_fh__nfs_file_handle__init(file_nfs_filehandle);
    *file_nfs_filehandle = deep_copy_nfs_filehandle(curr_fhandle->nfs_filehandle);
    file_fhandle->nfs_filehandle = file_nfs_filehandle;

    if(!is_first_component) {
        free(curr_fhandle->nfs_filehandle);
        free(curr_fhandle);
    }

    return file_fhandle;
}