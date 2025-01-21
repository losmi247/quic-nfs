#include "handlers.h" 

#include "src/repl/validation/validation.h"
#include "src/path_building/path_building.h"

#define READ_BYTES_PER_RPC 5000
#define SYMLINK_LEVELS_LIMIT 40

/*
* Resolves the given path to a file and gets its NFS filehandle by performing LOOKUP procedures
* from the filsystem root (for a given absolute path) or the current working directory (for a relative path),
* and returns that file's NFS fhandle and places that file's type in 'ftype' argument.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the received Nfs__FHandle and
* the NfsFh__NfsFileHandle inside it.
*/
Nfs__FHandle *resolve_path(char *path, Nfs__FType *ftype) {
    if(path == NULL) {
        return NULL;
    }

    if(path[0] == '\0') {
        return NULL;
    }

    Nfs__FHandle *curr_fhandle;
    if(path[0] == '/') {    // resolving an absolute path
        curr_fhandle = filesystem_dag_root->fhandle;
    }
    else {                  // resolving a relative path
        curr_fhandle = cwd_node->fhandle;
    }
    Nfs__FType curr_ftype = NFS__FTYPE__NFDIR;

    int pathnameLength = strlen(path);
    char *pathCopy = malloc(sizeof(char) *  pathnameLength);
    strncpy(pathCopy, path, pathnameLength);
    pathCopy[pathnameLength] = '\0';

    // look up component by component of the pathname
    char *save_ptr;
    char *pathname_component = strtok_r(pathCopy, "/", &save_ptr);
    while(pathname_component != NULL) {
        if(curr_ftype != NFS__FTYPE__NFDIR) {
            free(pathCopy);
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

            free(pathCopy);
            free(diropres);

            return NULL;
        }

        if(validate_nfs_dir_op_res(diropres) > 0) {
            printf("Error: Invalid NFS LOOKUP procedure result received from the server\n");

            free(pathCopy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }

        if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("cat: Permission denied\n");
            
            free(pathCopy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }
        else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
            printf("Error: Failed to lookup pathname component %s while resolving pathname %s, with status %s\n", pathname_component, path, string_status);
            free(string_status);

            free(pathCopy);
            nfs__dir_op_res__free_unpacked(diropres, NULL);

            return NULL;
        }
                
        curr_ftype = diropres->diropok->attributes->nfs_ftype->ftype;

        // make a stack allocated fhandle for this looked up file
        NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
        nfs__dir_op_res__free_unpacked(diropres, NULL);
        Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
        file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

        curr_fhandle = &file_fhandle;

        // get the next pathname component
        pathname_component = strtok_r(NULL, "/", &save_ptr);
    }

    free(pathCopy);

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
* of type 'start_file_type' until it encounters a file which is not a symbolic link
* and returns that file's Nfs fhandle.
*
* Returns NULL on failure.
*
* The user of this function takes the responsibility to free the received Nfs__FHandle and
* the NfsFh__NfsFileHandle inside it.
*/
Nfs__FHandle *follow_symbolic_links(Nfs__FHandle *start_file_fhandle, Nfs__FType start_file_type) {
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
            printf("cat: Permission denied\n");
            
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
        Nfs__FHandle *next_file_fhandle = resolve_path(target_path, &next_file_type);

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

/*
* Given a NFS FHandle of a file, performs a series of READ procedures
* to read out the entire file and prints it out.
*
* Returns 0 on success and > 0 on failure.
*/
int read_file(Nfs__FHandle *file_fhandle) {
    if(file_fhandle == NULL) {
        return 1;
    }

    uint64_t bytes_read = 0;
    uint8_t *read_data = NULL;
    uint64_t file_size = READ_BYTES_PER_RPC;    // we will know the file size after the first READ RPC
    while(bytes_read < file_size) {
        Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
        readargs.file = file_fhandle;
        readargs.offset = bytes_read;
        readargs.count = READ_BYTES_PER_RPC;

        readargs.totalcount = 0;

        Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
        int status = nfs_procedure_6_read_from_file(rpc_connection_context, readargs, readres);
        if(status != 0) {
            free(readres);
            free(read_data);

            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            return 1;
        }

        if(validate_nfs_read_res(readres) > 0) {
            printf("Error: Invalid NFS READ procedure result received from the server\n");

            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

            return 1;
        }

        if(readres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("cat: Permission denied\n");
            
            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

            return 1;
        }
        else if(readres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(readres->nfs_status->stat);
            printf("Error: Failed to read a file in the current working directory with status %s\n", string_status);
            free(string_status);

            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

            return 1;
        }

        // expand the buffer and save the read data
        read_data = realloc(read_data, sizeof(uint8_t) * (bytes_read + readres->readresbody->nfsdata.len));
        if(read_data == NULL) {
            printf("Error: Failed to allocate memory for the read data\n");
            return 1;
        }
        memcpy(read_data + bytes_read, readres->readresbody->nfsdata.data, readres->readresbody->nfsdata.len);

        file_size = readres->readresbody->attributes->size;
        bytes_read += readres->readresbody->nfsdata.len;

        nfs__read_res__free_unpacked(readres, NULL);
    }

    char *string_output = malloc(sizeof(char) * (bytes_read + 1));
    memcpy(string_output, read_data, bytes_read);
    string_output[bytes_read] = '\0';

    printf("%s", string_output);
    printf("\n");
    fflush(stdout);

    free(string_output);
    free(read_data);

    return 0;
}

/*
* Prints out the content of the given file.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_cat(char *file_name) {
    if(!is_filesystem_mounted()) {
        printf("Error: No remote file system is currently mounted\n");
        return 1;
    }

    // lookup the given file name
    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = file_name;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = cwd_node->fhandle;
    diropargs.name = &filename;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(rpc_connection_context, diropargs, diropres);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(diropres);

        return 1;
    }

    if(validate_nfs_dir_op_res(diropres) > 0) {
        printf("Error: Invalid NFS LOOKUP procedure result received from the server\n");

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("cat: Permission denied\n");
        
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }
    else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to lookup a file in the current working directory with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    // check that the file client wants to read is not a directory
    if(diropres->diropok->attributes->nfs_ftype->ftype == NFS__FTYPE__NFDIR) {
        printf("Error: Is a directory: %s\n", file_name);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    NfsFh__NfsFileHandle start_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    Nfs__FHandle start_file_fhandle = NFS__FHANDLE__INIT;
    start_file_fhandle.nfs_filehandle = &start_file_nfs_filehandle_copy;

    // follow a potential chain of symbolic links
    Nfs__FHandle *file_fhandle = follow_symbolic_links(&start_file_fhandle, diropres->diropok->attributes->nfs_ftype->ftype);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    if(file_fhandle == NULL) {
        return 1;
    }
    
    int error_code = read_file(file_fhandle);
    free(file_fhandle->nfs_filehandle);
    free(file_fhandle);
    return error_code;
}