#include "server.h"

#include "nfs_messages.h"

/*
* Nfs RPC program implementation.
*/

/*
* Runs the NFSPROC_NULL procedure (0).
*/
Rpc__AcceptedReply serve_nfs_procedure_0_do_nothing(Google__Protobuf__Any *parameters) {
    return wrap_procedure_results_in_successful_accepted_reply(0, NULL, "nfs/None");
}

/*
* Runs the NFSPROC_GETATTR procedure (1).
*/
Rpc__AcceptedReply serve_nfs_procedure_1_get_file_attributes(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: Expected nfs/FHandle but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__FHandle *fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(fhandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: Failed to unpack FHandle\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: FHandle->handle.data is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        return create_garbage_args_accepted_reply();
    }

    unsigned char *nfs_filehandle = fhandle->handle.data;
    ino_t inode_number = strtol(nfs_filehandle, NULL, 10);

    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode the inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such file or directory
        fprintf(stdout, "serve_nfs_procedure_1_get_file_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__fhandle__free_unpacked(fhandle, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file - we return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded inode number to a file
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: Failed getting file attributes with error code %d \n", error_code);

        nfs__fhandle__free_unpacked(fhandle, NULL);

        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;
    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

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
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/SAttrArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: Expected nfs/SAttrArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__SAttrArgs *sattrargs = nfs__sattr_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(sattrargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: Failed to unpack SAttrArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(sattrargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'file' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *fhandle = sattrargs->file;
    if(fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: FHandle->handle.data is null\n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(sattrargs->attributes == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'attributes' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__SAttr *sattr = sattrargs->attributes;
    if(sattr->atime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'atime' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(sattr->mtime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'mtime' in SAttrArgs is NULL \n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    unsigned char *nfs_filehandle = fhandle->handle.data;
    ino_t inode_number = strtol(nfs_filehandle, NULL, 10);
    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such file or directory
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    // update file attributes - -1 means don't update this attribute
    if(sattr->mode != -1 && chmod(file_absolute_path, sattr->mode) < 0) {
        perror("serve_nfs_procedure_2_set_file_attributes - Failed to update 'mode'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(chown(file_absolute_path, sattr->uid, sattr->gid) < 0) { // don't need to check if uid/gid is -1, as chown ignores uid or gid if it's -1
        perror("serve_nfs_procedure_2_set_file_attributes - Failed to update 'uid' and 'gid'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(sattr->size != -1 && truncate(file_absolute_path, sattr->size)) {
        perror("serve_nfs_procedure_2_set_file_attributes - Failed to update 'size'");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(sattr->atime->seconds != -1 && sattr->atime->useconds != -1 && sattr->mtime->seconds != -1 && sattr->mtime->useconds != -1) { // API only allows changing of both atime and mtime at once
        struct timeval times[2];
        times[0].tv_sec = sattr->atime->seconds;
        times[0].tv_usec = sattr->atime->useconds;
        times[1].tv_sec = sattr->mtime->seconds;
        times[1].tv_usec = sattr->mtime->useconds;

        if(utimes(file_absolute_path, times) < 0) {
            perror("serve_nfs_procedure_2_set_file_attributes - Failed to update 'atime' and 'mtime'");

            nfs__sattr_args__free_unpacked(sattrargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file - we return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded inode number to a file
        fprintf(stderr, "serve_procedure_2_set_file_attributes: Failed getting file attributes with error code %d \n", error_code);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;
    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

    nfs__sattr_args__free_unpacked(sattrargs, NULL);

    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    return accepted_reply;
}

/*
* Runs the NFSPROC_LOOKUP procedure (4).
*/
Rpc__AcceptedReply serve_nfs_procedure_4_look_up_file_name(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/DirOpArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: Expected nfs/DirOpArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__DirOpArgs *diropargs = nfs__dir_op_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(diropargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: Failed to unpack DirOpArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(diropargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'dir' in DirOpArgs is NULL \n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = diropargs->dir;   // we are given the NFS filehandle of the directory containing the file
    if(directory_fhandle->handle.data == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: FHandle->handle.data is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(diropargs->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'name' in DirOpArgs is NULL \n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = diropargs->name;
    if(file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'filename' in DirOpArgs is NULL \n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    unsigned char *directory_nfs_filehandle = directory_fhandle->handle.data;
    ino_t inode_number = strtol(directory_nfs_filehandle, NULL, 10);
    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }

    // absolute path of the looked up file is directory_path/file_name
    int file_absolute_path_length = strlen(directory_absolute_path) + 1 + strlen(file_name->filename);
    char *concatenation_buffer = malloc(file_absolute_path_length + 1);   // create a string with enough space, +1 for termination character
    strcpy(concatenation_buffer, directory_absolute_path);    // move the directory absolute path there
    concatenation_buffer = strcat(concatenation_buffer, "/"); // add a slash at end
    char *file_absolute_path = strcat(concatenation_buffer, file_name->filename);
    // create a NFS filehandle for the looked up file
    uint8_t *file_nfs_filehandle = malloc(sizeof(uint8_t) * FHSIZE);
    int error_code = create_nfs_filehandle(file_absolute_path, file_nfs_filehandle, &inode_cache);
    if(error_code == 1) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: creation of nfs filehandle failed with error code %d \n", error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        free(file_nfs_filehandle);

        free(concatenation_buffer);

        return create_system_error_accepted_reply();
    }
    if(error_code == 2) {
        // the file that the client wants to look up in this directory does not exist
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: Failed getting file attributes with error code %d \n", error_code);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        free(diropres->default_case);
        free(diropres);

        Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        free(file_nfs_filehandle);

        free(concatenation_buffer);

        return accepted_reply;
    }

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.handle.data = file_nfs_filehandle;
    file_fhandle.handle.len = strlen(file_nfs_filehandle) + 1; // +1 for the null termination!

    // get the attributes of the looked up file
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: Failed getting file attributes with error code %d \n", error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        free(file_nfs_filehandle);

        free(concatenation_buffer);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've created a NFS filehandle for this file (we successfully read stat.st_ino)
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__DirOpRes diropres = NFS__DIR_OP_RES__INIT;
    diropres.status = NFS__STAT__NFS_OK;
    diropres.body_case = NFS__DIR_OP_RES__BODY_DIROPOK;

    Nfs__DirOpOk diropok = NFS__DIR_OP_OK__INIT;
    diropok.file = &file_fhandle;
    diropok.attributes = &fattr;

    diropres.diropok = &diropok;

    // serialize the procedure results
    size_t diropres_size = nfs__dir_op_res__get_packed_size(&diropres);
    uint8_t *diropres_buffer = malloc(diropres_size);
    nfs__dir_op_res__pack(&diropres, diropres_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");

    nfs__dir_op_args__free_unpacked(diropargs, NULL);

    free(file_nfs_filehandle);

    free(concatenation_buffer);

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
        // procedure 3 - NFSPROC_ROOT is obsolete
        case 4:
            return serve_nfs_procedure_4_look_up_file_name(parameters);
        case 5:
        default:
    }

    // procedure not found
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROC_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
    return accepted_reply;
}