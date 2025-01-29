#include "soft_links.h"

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

#define SYMLINK_LEVELS_LIMIT 40

/*
* Given the PRC connection context, DAGNode of the filesystem root and the 
* DAGNode of the current working directory, resolves the given path (absolute or relative)
* to a file as specified in https://man7.org/linux/man-pages/man7/path_resolution.7.html.
*
* Performs LOOKUP procedures from the filesystem root (for a given absolute path) or
* the current working directory (for a relative path) to resolve component by component of the
* pathname. Returns the NFS fhandle of the resolved file and places that file's type in 'ftype' argument.
*
* The 'command_name' argument is used for indicating which command invoked this path resolution,
* when an error occurs.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the received Nfs__FHandle and
* the NfsFh__NfsFileHandle inside it.
*/
Nfs__FHandle *resolve_path(char *command_name, RpcConnectionContext *rpc_connection_context, DAGNode *filesystem_dag_root, DAGNode *cwd_node, char *path, Nfs__FType *ftype) {
    if(command_name == NULL) {
        return NULL;
    }
    
    if(rpc_connection_context == NULL) {
        return NULL;
    }

    if(filesystem_dag_root == NULL || cwd_node == NULL) {
        return NULL;
    }

    if(path == NULL) {
        return NULL;
    }

    if(path[0] == '\0') {
        return NULL;
    }

    Nfs__FHandle *curr_fhandle;
    if(path[0] == '/') {    // we are resolving an absolute path
        curr_fhandle = filesystem_dag_root->fhandle;
    }
    else {                  // we are resolving a relative path
        curr_fhandle = cwd_node->fhandle;
    }
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
            printf("%s: Permission denied\n", command_name);
            
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

    return file_fhandle;
}

/*
* Follows the chain of symbolic links starting from the given 'start_file_fhandle' 
* of type 'start_file_type' until it encounters a file which is not a symbolic link and
* returns that file's Nfs fhandle.
*
* The given RpcConnectionContext is used for making RPCs, and the given filesystem root DAGNode
* and the current working directory DAGNode are used for resolving paths contained in the symbolic
* links.
*
* The 'command_name' argument is used for indicating which command invoked this following
* of symbolic links, when an error occurs.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the received Nfs__FHandle and
* the NfsFh__NfsFileHandle inside it.
*/
Nfs__FHandle *follow_symbolic_links(char *command_name, RpcConnectionContext *rpc_connection_context, DAGNode *filesystem_dag_root, DAGNode *cwd_node, Nfs__FHandle *start_file_fhandle, Nfs__FType start_file_type) {
    if(command_name == NULL) {
        return NULL;
    }
    
    if(rpc_connection_context == NULL) {
        return NULL;
    }

    if(filesystem_dag_root == NULL || cwd_node == NULL) {
        return NULL;
    }

    if(start_file_fhandle == NULL) {
        return NULL;
    }

    int symlink_levels = 0;
    Nfs__FHandle *curr_fhandle = start_file_fhandle;
    Nfs__FType curr_file_type = start_file_type;
    while(curr_file_type == NFS__FTYPE__NFLNK) {
        if(symlink_levels == SYMLINK_LEVELS_LIMIT) {
            printf("Error: Too many levels of symbolic links\n");

            return NULL;
        }

        Nfs__ReadLinkRes *readlinkres = malloc(sizeof(Nfs__ReadLinkRes));
        int status = nfs_procedure_5_read_from_symbolic_link(rpc_connection_context, *curr_fhandle, readlinkres);
        if(status != 0) {
            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            free(readlinkres);

            return NULL;
        }

        if(validate_nfs_read_link_res(readlinkres) > 0) {
            printf("Error: Invalid NFS READLINK procedure result received from the server\n");

            nfs__read_link_res__free_unpacked(readlinkres, NULL);

            return NULL;
        }

        if(readlinkres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("%s: Permission denied\n", command_name);
            
            nfs__read_link_res__free_unpacked(readlinkres, NULL);

            return NULL;
        }
        else if(readlinkres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(readlinkres->nfs_status->stat);
            printf("Error: Failed to read from a symbolic link in the current working directory with status %s\n", string_status);
            free(string_status);

            nfs__read_link_res__free_unpacked(readlinkres, NULL);

            return NULL;
        }

        char *target_path = strdup(readlinkres->data->path);
        nfs__read_link_res__free_unpacked(readlinkres, NULL);

        // free the current fhandle in case it was produced by 'resolve_path'
        if(curr_fhandle != start_file_fhandle) {
            free(curr_fhandle->nfs_filehandle);
            free(curr_fhandle);
        }

        // resolve the given path
        Nfs__FType next_file_type;
        Nfs__FHandle *next_file_fhandle = resolve_path(command_name, rpc_connection_context, filesystem_dag_root, cwd_node,target_path, &next_file_type);

        symlink_levels++;
        curr_fhandle = next_file_fhandle;
        curr_file_type = next_file_type;
    }

    // make a heap-allocated copy of the NFS fhandle of the file at the end of the chain
    Nfs__FHandle *end_file_fhandle = malloc(sizeof(Nfs__FHandle));
    nfs__fhandle__init(end_file_fhandle);
    NfsFh__NfsFileHandle *end_file_nfs_filehandle = malloc(sizeof(NfsFh__NfsFileHandle));
    nfs_fh__nfs_file_handle__init(end_file_nfs_filehandle);
    *end_file_nfs_filehandle = deep_copy_nfs_filehandle(curr_fhandle->nfs_filehandle);
    end_file_fhandle->nfs_filehandle = end_file_nfs_filehandle;

    // free the last curr_fhandle if it's not the start_file_fhandle
    if(curr_fhandle != start_file_fhandle) {
        free(curr_fhandle->nfs_filehandle);
        free(curr_fhandle);
    }

    return end_file_fhandle;
}