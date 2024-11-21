#include "server.h"

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