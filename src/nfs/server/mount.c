#include "server.h"

/*
* Mount RPC program implementation.
*/

/*
* Exported files management.
*/
int is_directory_exported(const char *absolute_path) {
    if(absolute_path == NULL) {
        return 0;
    }

    FILE *file = fopen("./exports", "r");
    if(file == NULL) {
        fprintf(stderr, "Mount server failed to open /etc/exports\n");
        return 0;
    }

    int is_exported = 0;
    char line[1024];
    // read file line by line
    while(fgets(line, sizeof(line), file) != NULL) {
        // remove trailing newline if present
        line[strcspn(line, "\n")] = '\0';

        // check if the line starts with absolute_path
        if (strncmp(line, absolute_path, strlen(absolute_path)) == 0) {
            const char *options_start = line + strlen(absolute_path);
            // check for whitespace after the path
            if((*options_start == ' ' || *options_start == '\t') && strstr(options_start, "*(rw)") != NULL) {
                is_exported = 1;
                break;
            }
        }
    }

    fclose(file);

    return is_exported;
}

/*
* Creates a NFS filehandle for the given absolute path of a directory being mounted, and returns
* it in the 'nfs_filehandle' argument.
*
* Returns 0 on success and > 0 on failure. TODO: concatenate to this a UNIX timestamp
*/
int create_nfs_filehandle(char *directory_absolute_path, unsigned char *nfs_filehandle) {
    if(directory_absolute_path == NULL) {
        nfs_filehandle = NULL;
        return 1;
    }

    ino_t inode_number;
    get_inode_number(directory_absolute_path, &inode_number);

    // remember what absolute path this inode number corresponds to
    add_inode_mapping(inode_number, directory_absolute_path, &inode_cache);

    sprintf(nfs_filehandle, "%lu", inode_number);
    
    return 0;
}

/*
* Runs the MOUNTPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_mnt_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // return an Any with empty buffer inside it
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "mount/None";
    results->value.data = NULL;
    results->value.len = 0;

    accepted_reply.results = results;

    return accepted_reply;
}

/*
* Runs the MOUNTPROC_MNT procedure (1).
*/
Rpc__AcceptedReply serve_mnt_procedure_1_add_mount_entry(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "mount/DirPath") != 0) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: Expected mount/DirPath but received %s\n", parameters->type_url);
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // deserialize parameters
    // if valid, this DirPath is freed at server shutdown as part of mountlist cleanup - it needs to be persisted here at server!
    Mount__DirPath *dirpath = mount__dir_path__unpack(NULL, parameters->value.len, parameters->value.data);
    if(dirpath == NULL) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: Failed to unpack DirPath\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(dirpath->path == NULL) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: DirPath->path is null\n");

        mount__dir_path__free_unpacked(dirpath, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // check the server has exported this directory for NFS
    if(!is_directory_exported(dirpath->path)) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: directory %s not exported for NFS\n", dirpath->path);

        mount__dir_path__free_unpacked(dirpath, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // create a file handle for this directory
    uint8_t *filehandle = malloc(sizeof(uint8_t) * 32);
    int error_code = create_nfs_filehandle(dirpath->path, filehandle);
    if(error_code > 0) {
        fprintf(stderr, "serve_procedure_1_add_mount_entry: creation of nfs filehandle failed with error code %d \n", error_code);

        mount__dir_path__free_unpacked(dirpath, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // create a new mount entry - pass *dirpath instead of dirpath so that we are able to free later
    Mount__MountList *new_mount_entry = create_new_mount_entry(strdup("client-hostname"), *dirpath); // replace hostname with actual client hostname when available
    add_mount_entry(&mount_list, new_mount_entry);

    // build the procedure results
    Mount__FhStatus fh_status = MOUNT__FH_STATUS__INIT;
    fh_status.status = 0;
    fh_status.fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY;

    Mount__FHandle fhandle = MOUNT__FHANDLE__INIT;
    fhandle.handle.data = filehandle;
    fhandle.handle.len = strlen(filehandle) + 1; // +1 for the null termination!

    fh_status.directory = &fhandle;

    // serialize the procedure results
    size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
    uint8_t *fh_status_buffer = malloc(fh_status_size);
    mount__fh_status__pack(&fh_status, fh_status_buffer);

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "mount/FhStatus";
    results->value.data = fh_status_buffer;
    results->value.len = fh_status_size;

    // complete the AcceptedReply
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;
    accepted_reply.results = results;

    // do not mount__dir_path__free_unpacked(dirpath, NULL); here - it will be freed when mount list is cleaned up on shut down

    return accepted_reply;
}

/*
* Calls the appropriate procedure in the Mount RPC program based on the procedure number.
*/
Rpc__AcceptedReply call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_version != 2) {
        Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

        // accepted_reply contains a pointer to mismatch_info, so mismatch_info has to be heap allocated - because it's going to be used outside of scope of this function
        Rpc__MismatchInfo *mismatch_info = malloc(sizeof(Rpc__MismatchInfo));
        rpc__mismatch_info__init(mismatch_info);
        mismatch_info->low = 2;
        mismatch_info->high = 2;

        accepted_reply.stat = RPC__ACCEPT_STAT__PROG_MISMATCH;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_MISMATCH_INFO;
        accepted_reply.mismatch_info = mismatch_info;

        return accepted_reply;
    }

    switch(procedure_number) {
        case 0:
            return serve_mnt_procedure_0_do_nothing(parameters);
        case 1:
            return serve_mnt_procedure_1_add_mount_entry(parameters);
        case 2:
        case 3:
        case 4:
        case 5:
        default:
    }

    // procedure not found
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROC_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
    return accepted_reply;
}