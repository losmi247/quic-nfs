#include "handlers.h" 

#include "src/path_building/path_building.h"

#include "src/repl/soft_links/soft_links.h"

#define READ_BYTES_PER_RPC 5000

/*
* Given a NFS FHandle of a file, performs a series of READ procedures
* to read out the entire file and prints it out.
*
* Returns 0 on success and > 0 on failure.
*/
int print_file(Nfs__FHandle *file_fhandle) {
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

        readargs.totalcount = 0;    // unused field

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
    Nfs__FHandle *file_fhandle = follow_symbolic_links("cat", rpc_connection_context, filesystem_dag_root, cwd_node, &start_file_fhandle, diropres->diropok->attributes->nfs_ftype->ftype);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    if(file_fhandle == NULL) {
        return 1;
    }
    
    int error_code = print_file(file_fhandle);
    free(file_fhandle->nfs_filehandle);
    free(file_fhandle);
    return error_code;
}