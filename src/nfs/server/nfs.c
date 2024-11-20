#define _POSIX_C_SOURCE 200809L // to be able to use truncate() in unistd.h

#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <unistd.h> // read(), write(), close()
#include <utime.h> // changing atime, ctime
#include <signal.h> // for registering the SIGTERM handler

#include "src/common_rpc/server_common_rpc.h"

#include <protobuf-c/protobuf-c.h>
#include "src/serialization/rpc/rpc.pb-c.h"
#include "src/serialization/nfs/nfs.pb-c.h"
#include "src/serialization/mount/mount.pb-c.h"

#include "../nfs_common.h"

#include "./mount_list.h"
#include "./inode_cache.h"

#include "src/file_management/file_management.h"

Rpc__AcceptedReply call_mount(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

Rpc__AcceptedReply call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters);

/*
* Functions from server_common_rpc.h that each RPC program's server must implement.
*/

Rpc__AcceptedReply forward_rpc_call_to_program(uint32_t program_number, uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
    if(program_number == MOUNT_RPC_PROGRAM_NUMBER) {
        return call_mount(program_version, procedure_number, parameters);
    }
    if(program_number == NFS_RPC_PROGRAM_NUMBER) {
        return call_nfs(program_version, procedure_number, parameters);
    }

    fprintf(stderr, "Unknown program number");
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROG_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    accepted_reply.default_case = empty;

    return accepted_reply;
}

/*
* Mount+Nfs RPC program state
*/

int rpc_server_socket_fd;
InodeCache inode_cache;

/*
* Mount RPC program implementation.
*/

Mount__MountList *mount_list;

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


/*
* Nfs RPC program implementation.
*/

/*
* Runs the NFSPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_nfs_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;

    // return an Any with empty buffer inside it
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "nfs/None";
    results->value.data = NULL;
    results->value.len = 0;

    accepted_reply.results = results;

    return accepted_reply;
}

/*
* Runs the NFSPROC_GETATTR procedure (1).
*/
Rpc__AcceptedReply serve_nfs_procedure_1_get_file_attributes(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: Expected nfs/FHandle but received %s\n", parameters->type_url);
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // deserialize parameters
    Nfs__FHandle *fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(fhandle == NULL) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: Failed to unpack FHandle\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: FHandle->handle.data is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    unsigned char *nfs_filehandle = fhandle->handle.data;
    ino_t inode_number = strtol(nfs_filehandle, NULL, 10);

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory
        fprintf(stderr, "serve_procedure_1_get_file_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        nfs__fhandle__free_unpacked(fhandle, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_procedure_1_get_file_attributes: Failed getting file attributes\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "nfs/AttrStat";
    results->value.data = attr_stat_buffer;
    results->value.len = attr_stat_size;

    // complete the AcceptedReply
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;
    accepted_reply.results = results;

    nfs__fhandle__free_unpacked(fhandle, NULL);

    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    return accepted_reply;
}

/*
* Runs the NFSPROC_SETATTR procedure (2).
*/
Rpc__AcceptedReply serve_nfs_procedure_2_set_file_attributes(Google__Protobuf__Any *parameters) {
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;

    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/SAttrArgs") != 0) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: Expected nfs/SAttrArgs but received %s\n", parameters->type_url);
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // deserialize parameters
    Nfs__SAttrArgs *sattrargs = nfs__sattr_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(sattrargs == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: Failed to unpack SAttrArgs\n");
        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(sattrargs->file == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: 'file' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    Nfs__FHandle *fhandle = sattrargs->file;
    if(fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: FHandle->handle.data is null\n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(sattrargs->attributes == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: 'attributes' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    Nfs__SAttr *sattr = sattrargs->attributes;
    if(sattr->atime == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: 'atime' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(sattr->mtime == NULL) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: 'mtime' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__GARBAGE_ARGS;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    unsigned char *nfs_filehandle = fhandle->handle.data;
    ino_t inode_number = strtol(nfs_filehandle, NULL, 10);
    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory
        fprintf(stderr, "serve_procedure_2_set_file_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    // update file attributes - -1 means don't update this attribute
    if(sattr->mode != -1 && chmod(file_absolute_path, sattr->mode) < 0) {
        perror("serve_procedure_2_set_file_attributes - Failed to update 'mode'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(chown(file_absolute_path, sattr->uid, sattr->gid) < 0) { // don't need to check if uid/gid is -1, as chown ignores uid or gid if it's -1
        perror("serve_procedure_2_set_file_attributes - Failed to update 'uid' and 'gid'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(sattr->size != -1 && truncate(file_absolute_path, sattr->size)) {
        perror("serve_procedure_2_set_file_attributes - Failed to update 'size'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }
    if(sattr->atime->seconds != -1 && sattr->atime->useconds != -1 && sattr->mtime->seconds != -1 && sattr->mtime->useconds != -1) { // API only allows changing of both atime and mtime at once
        struct timeval times[2];
        times[0].tv_sec = sattr->atime->seconds;
        times[0].tv_usec = sattr->atime->useconds;
        times[1].tv_sec = sattr->mtime->seconds;
        times[1].tv_usec = sattr->mtime->useconds;

        if(utimes(file_absolute_path, times) < 0) {
            perror("serve_procedure_2_set_file_attributes - Failed to update 'atime' and 'mtime'");

            nfs__sattr_args__free_unpacked(sattrargs, NULL);

            accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
            accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
            return accepted_reply;
        }
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;

    Nfs__FAttr fattr = NFS__FATTR__INIT;

    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: Failed getting file attributes\n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        accepted_reply.stat = RPC__ACCEPT_STAT__SYSTEM_ERR;
        accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
        return accepted_reply;
    }

    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    // wrap procedure results into Any
    Google__Protobuf__Any *results = malloc(sizeof(Google__Protobuf__Any));
    google__protobuf__any__init(results);
    results->type_url = "nfs/AttrStat";
    results->value.data = attr_stat_buffer;
    results->value.len = attr_stat_size;

    // complete the AcceptedReply
    accepted_reply.stat = RPC__ACCEPT_STAT__SUCCESS;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_RESULTS;
    accepted_reply.results = results;

    nfs__sattr_args__free_unpacked(sattrargs, NULL);

    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    return accepted_reply;
}

/*
* Calls the appropriate procedure in the Nfs RPC program based on the procedure number.
*/
Rpc__AcceptedReply call_nfs(uint32_t program_version, uint32_t procedure_number, Google__Protobuf__Any *parameters) {
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
            return serve_nfs_procedure_0_do_nothing(parameters);
        case 1:
            return serve_nfs_procedure_1_get_file_attributes(parameters);
        case 2:
            return serve_nfs_procedure_2_set_file_attributes(parameters);
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

/*
* Signal handler for graceful shutdown
*/
void handle_signal(int signal) {
    if (signal == SIGTERM) {
        fprintf(stdout, "Received SIGTERM, shutting down gracefully...\n");
        
        close(rpc_server_socket_fd);

        exit(0);
    }
}

/*
* The main body of the Nfs+Mount server, which awaits RPCs.
*/
int main() {
    signal(SIGTERM, handle_signal);  // register signal handler

    rpc_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(rpc_server_socket_fd < 0) { 
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    }

    // initialize Nfs and Mount server state
    mount_list = NULL;
    inode_cache = NULL;

    struct sockaddr_in rpc_server_addr;
    rpc_server_addr.sin_family = AF_INET; 
    rpc_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rpc_server_addr.sin_port = htons(NFS_RPC_SERVER_PORT);
  
    // bind socket to the rpc server address
    if(bind(rpc_server_socket_fd, (struct sockaddr *) &rpc_server_addr, sizeof(rpc_server_addr)) < 0) { 
        fprintf(stderr, "Socket bind failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }
  
    // listen for connections on the port
    if(listen(rpc_server_socket_fd, 10) < 0) {
        fprintf(stderr, "Listen failed\n");
        close(rpc_server_socket_fd);
        return 1;
    }

    fprintf(stdout, "Server listening on port %d\n", NFS_RPC_SERVER_PORT);
  
    while(1) {
        struct sockaddr_in rpc_client_addr;
        socklen_t rpc_client_addr_len = sizeof(rpc_client_addr);

        int rpc_client_socket_fd = accept(rpc_server_socket_fd, (struct sockaddr *) &rpc_client_addr, &rpc_client_addr_len);
        if(rpc_client_socket_fd < 0) { 
            fprintf(stderr, "Server failed to accept connection\n"); 
            continue;
        }
    
        handle_client(rpc_client_socket_fd);

        close(rpc_client_socket_fd);
    }
     
    close(rpc_server_socket_fd);
} 