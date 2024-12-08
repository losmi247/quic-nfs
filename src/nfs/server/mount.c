#include "server.h"

#include "mount_messages.h"

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
* Runs the MOUNTPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_mnt_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "mount/None");
}

/*
* Runs the MOUNTPROC_MNT procedure (1).
*/
Rpc__AcceptedReply serve_mnt_procedure_1_add_mount_entry(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "mount/DirPath") != 0) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: Expected mount/DirPath but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    // if valid, this DirPath is freed at server shutdown as part of mountlist cleanup - it needs to be persisted here at server!
    Mount__DirPath *dirpath = mount__dir_path__unpack(NULL, parameters->value.len, parameters->value.data);
    if(dirpath == NULL) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: Failed to unpack DirPath\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(dirpath->path == NULL) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: DirPath->path is null\n");

        mount__dir_path__free_unpacked(dirpath, NULL);

        return create_garbage_args_accepted_reply();
    }
    char *directory_absolute_path = dirpath->path;

    // check that the directory client wants to mount exists
    int error_code = access(directory_absolute_path, F_OK);
    if(error_code < 0) {
        if(errno == EIO || errno == ENOENT) {
            Mount__Stat mount_stat;
            switch(errno) {
                case EIO:
                    mount_stat = MOUNT__STAT__MNTERR_IO;
                    fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: physical IO error occurred while checking if directory at absolute path '%s' exists\n", directory_absolute_path);
                case ENOENT:
                    mount_stat = MOUNT__STAT__MNTERR_NOENT;
                    fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: attempted to mount a directory at absolute path '%s' which does not exist\n", directory_absolute_path);
            }

            // build the procedure results
            Mount__FhStatus fh_status = create_default_case_fh_status(mount_stat);

            // serialize the procedure results
            size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
            uint8_t *fh_status_buffer = malloc(fh_status_size);
            mount__fh_status__pack(&fh_status, fh_status_buffer);

            Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(fh_status_size, fh_status_buffer, "mount/FhStatus");

            mount__dir_path__free_unpacked(dirpath, NULL);
            free(fh_status.default_case);

            return accepted_reply;
        }
        else{
            perror_msg("serve_mnt_procedure_1_add_mount_entry: failed checking if directory to be mounted at absolute path '%s' exists", directory_absolute_path);

            mount__dir_path__free_unpacked(dirpath, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // get the attributes of this directory, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: failed getting file attributes for file at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        mount__dir_path__free_unpacked(dirpath, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've checked this file/directory exists
        return create_system_error_accepted_reply();
    }
    // only directories can be mounted using MNT
    if(directory_fattr.type != NFS__FTYPE__NFDIR) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: 'mnt' procedure called on a non-directory '%s'\n", directory_absolute_path);

        // build the procedure results
        Mount__FhStatus fh_status = create_default_case_fh_status(MOUNT__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
        uint8_t *fh_status_buffer = malloc(fh_status_size);
        mount__fh_status__pack(&fh_status, fh_status_buffer);

        Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(fh_status_size, fh_status_buffer, "mount/FhStatus");

        mount__dir_path__free_unpacked(dirpath, NULL);
        free(fh_status.default_case);

        return accepted_reply;
    }
    free(directory_fattr.atime);
    free(directory_fattr.mtime);
    free(directory_fattr.ctime);

    if(!is_directory_exported(directory_absolute_path)) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: directory at absolute path '%s' not exported for Nfs\n", directory_absolute_path);

        Mount__FhStatus fh_status = create_default_case_fh_status(MOUNT__STAT__MNTERR_NOTEXP);

        // serialize the procedure results
        size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
        uint8_t *fh_status_buffer = malloc(fh_status_size);
        mount__fh_status__pack(&fh_status, fh_status_buffer);

        free(fh_status.default_case);

        Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(fh_status_size, fh_status_buffer, "mount/FhStatus");

        mount__dir_path__free_unpacked(dirpath, NULL);

        return accepted_reply;
    }

    // create a NFS file handle for this directory
    NfsFh__NfsFileHandle directory_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    error_code = create_nfs_filehandle(directory_absolute_path, &directory_nfs_filehandle, &inode_cache);
    if(error_code > 0) {
        fprintf(stderr, "serve_mnt_procedure_1_add_mount_entry: failed creating a NFS filehandle for directory at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        mount__dir_path__free_unpacked(dirpath, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've checked that the looked up file exists
        return create_system_error_accepted_reply();
    }

    // create a new mount entry - pass *dirpath instead of dirpath so that we are able to free it later from the mount list
    add_mount_list_entry("client-hostname", directory_absolute_path, &mount_list); // TODO (QNFS-20): replace 'client-hostname' with actual client hostname when available

    // build the procedure results
    Mount__FhStatus fh_status = MOUNT__FH_STATUS__INIT;
    fh_status.status = MOUNT__STAT__MNT_OK;
    fh_status.fhstatus_body_case = MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY;

    Mount__FHandle fhandle = MOUNT__FHANDLE__INIT;
    fhandle.nfs_filehandle = &directory_nfs_filehandle;

    fh_status.directory = &fhandle;

    // serialize the procedure results
    size_t fh_status_size = mount__fh_status__get_packed_size(&fh_status);
    uint8_t *fh_status_buffer = malloc(fh_status_size);
    mount__fh_status__pack(&fh_status, fh_status_buffer);

    mount__dir_path__free_unpacked(dirpath, NULL);

    return wrap_procedure_results_in_successful_accepted_reply(fh_status_size, fh_status_buffer, "mount/FhStatus");
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