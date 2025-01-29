#include "handlers.h"

#include "src/path_building/path_building.h"

#include "src/repl/soft_links/soft_links.h"

#define WRITE_BYTES_PER_RPC 5000

/*
* Given a NFS FHandle of a file, performs a series of WRITE procedures
* to append the given text to the end of the file in a new line.
*
* Returns 0 on success and > 0 on failure.
*/
int append_to_file(Nfs__FHandle *file_fhandle, char *text) {
    if(file_fhandle == NULL) {
        return 1;
    }

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(rpc_connection_context, *file_fhandle, attrstat);
    if(status != 0) {
        free(attrstat);

        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        return 1;
    }

    if(validate_nfs_attr_stat(attrstat) > 0) {
        printf("Error: Invalid NFS READ procedure result received from the server\n");

        nfs__attr_stat__free_unpacked(attrstat, NULL);

        return 1;
    }

    if(attrstat->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("echo: Permission denied\n");
        
        nfs__attr_stat__free_unpacked(attrstat, NULL);

        return 1;
    }
    else if(attrstat->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(attrstat->nfs_status->stat);
        printf("Error: Failed to get file attributes of a file in the current working directory with status %s\n", string_status);
        free(string_status);

        nfs__attr_stat__free_unpacked(attrstat, NULL);

        return 1;
    }

    // remember the file size so that we can append to the file
    uint64_t original_file_size = attrstat->attributes->size;
    nfs__attr_stat__free_unpacked(attrstat, NULL);

    // add a newline to the start of the text we want to write
    size_t text_length = strlen(text);
    char *text_with_newline = malloc(sizeof(char) * (text_length + 2));
    text_with_newline[0] = '\n';
    memcpy(text_with_newline + 1, text, text_length);
    text_with_newline[text_length + 1] = '\0';
    text_length = text_length + 1;

    uint64_t bytes_written = 0;
    while(bytes_written < text_length) {
        Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
        writeargs.file = file_fhandle;
        writeargs.offset = original_file_size + bytes_written;

        writeargs.nfsdata.data = text_with_newline + bytes_written;
        size_t bytes_left = text_length - bytes_written;
        if(bytes_left < WRITE_BYTES_PER_RPC) {
            writeargs.nfsdata.len = bytes_left;
        }
        else {
            writeargs.nfsdata.len = WRITE_BYTES_PER_RPC;
        }

        writeargs.beginoffset = writeargs.totalcount = 0;   // unused fields

        Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
        status = nfs_procedure_8_write_to_file(rpc_connection_context, writeargs, attrstat);
        if(status != 0) {
            free(attrstat);

            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            return 1;
        }

        if(validate_nfs_attr_stat(attrstat) > 0) {
            printf("Error: Invalid NFS READ procedure result received from the server\n");

            nfs__attr_stat__free_unpacked(attrstat, NULL);

            return 1;
        }

        if(attrstat->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("echo: Permission denied\n");
            
            nfs__attr_stat__free_unpacked(attrstat, NULL);

            return 1;
        }
        else if(attrstat->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(attrstat->nfs_status->stat);
            printf("Error: Failed to write to a file in the current working directory with status %s\n", string_status);
            free(string_status);

            nfs__attr_stat__free_unpacked(attrstat, NULL);

            return 1;
        }

        bytes_written += writeargs.nfsdata.len;

        nfs__attr_stat__free_unpacked(attrstat, NULL);
    }

    free(text_with_newline);

    return 0;
}

/*
* Writes the given text into the the given file in the cwd.
*
* Returns 0 on success and > 0 on failure.
*/
int handle_echo(char *text, char *file_name) {
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
        printf("echo: Permission denied\n");
        
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

    // check that the file client wants to write to is not a directory
    if(diropres->diropok->attributes->nfs_ftype->ftype == NFS__FTYPE__NFDIR) {
        printf("Error: Is a directory: %s\n", file_name);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        return 1;
    }

    NfsFh__NfsFileHandle start_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    Nfs__FHandle start_file_fhandle = NFS__FHANDLE__INIT;
    start_file_fhandle.nfs_filehandle = &start_file_nfs_filehandle_copy;

    // follow a potential chain of symbolic links
    Nfs__FHandle *file_fhandle = follow_symbolic_links("echo", rpc_connection_context, filesystem_dag_root, cwd_node, &start_file_fhandle, diropres->diropok->attributes->nfs_ftype->ftype);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    if(file_fhandle == NULL) {
        return 1;
    }
    
    int error_code = append_to_file(file_fhandle, text);
    free(file_fhandle->nfs_filehandle);
    free(file_fhandle);
    return error_code;
}