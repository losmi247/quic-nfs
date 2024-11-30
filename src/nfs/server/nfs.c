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
    if(fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: FHandle->nfs_filehandle is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *nfs_filehandle = fhandle->nfs_filehandle;
    ino_t inode_number = nfs_filehandle->inode_number;

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
        // we failed getting attributes for this file/directory - we return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded inode number to a file
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

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
    if(fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: FHandle->nfs_filehandle is null\n");

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

    NfsFh__NfsFileHandle *nfs_filehandle = fhandle->nfs_filehandle;
    ino_t inode_number = nfs_filehandle->inode_number;

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
    // TODO (QNFS-21) - change this to either update all arguments or none (don't want a partial update)
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
    if(sattr->size != -1 && truncate(file_absolute_path, sattr->size) < 0) {
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
        fprintf(stderr, "serve_procedure_2_set_file_attributes: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

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
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: FHandle->nfs_filehandle is null\n");

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

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

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

    // get the attributes of this directory before the looku[], to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed getting file attributes for file at absolute path '%s' with error code %d \n", directory_absolute_path, error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    // only directories can be looked up using LOOKUP
    if(directory_fattr.type != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return DirOpRes with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'lookup' procedure called on a non-directory '%s'\n", directory_absolute_path);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }

    char *file_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    // create a NFS filehandle for the looked up file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    error_code = create_nfs_filehandle(file_absolute_path, &file_nfs_filehandle, &inode_cache);
    if(error_code == 1) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: creation of nfs filehandle failed with error code %d \n", error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        free(file_absolute_path);

        return create_system_error_accepted_reply();
    }
    if(error_code == 2) {
        // the file that the client wants to look up in this directory does not exist
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

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

        free(file_absolute_path);

        return accepted_reply;
    }

    // get the attributes of the looked up file
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        free(file_absolute_path);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've created a NFS filehandle for this file (we successfully read stat.st_ino)
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__DirOpRes diropres = NFS__DIR_OP_RES__INIT;
    diropres.status = NFS__STAT__NFS_OK;
    diropres.body_case = NFS__DIR_OP_RES__BODY_DIROPOK;

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

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

    // do not free(file_absolute_path) here - it is in the inode cache and will be freed when inode cache is freed on server shut down

    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    return accepted_reply;
}

/*
* Runs the NFSPROC_READ procedure (6).
*/
Rpc__AcceptedReply serve_nfs_procedure_6_read_from_file(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/ReadArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: Expected nfs/ReadArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__ReadArgs *readargs = nfs__read_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(readargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: Failed to unpack ReadArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(readargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: 'file' in ReadArgs is NULL \n");

        nfs__read_args__free_unpacked(readargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *file_fhandle = readargs->file;
    if(file_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: FHandle->nfs_filehandle is null\n");

        nfs__read_args__free_unpacked(readargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *file_nfs_filehandle = file_fhandle->nfs_filehandle;
    ino_t inode_number = file_nfs_filehandle->inode_number;

    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file - we assume the client gave us a wrong NFS filehandle, i.e. no such file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed to decode inode number %ld back to a file\n", inode_number);

        // build the procedure results
        Nfs__ReadRes *readres = create_default_case_read_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t readres_size = nfs__read_res__get_packed_size(readres);
        uint8_t *readres_buffer = malloc(readres_size);
        nfs__read_res__pack(readres, readres_buffer);

        nfs__read_args__free_unpacked(readargs, NULL);
        free(readres->default_case);
        free(readres);

        return wrap_procedure_results_in_successful_accepted_reply(readres_size, readres_buffer, "nfs/ReadRes");
    }

    // get the attributes of the looked up file before the read, to check that the file is not a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // all file types except for directories can be read as files
    if(fattr.type == NFS__FTYPE__NFDIR) {
        // if the file is actually directory, return ReadRes with 'directory specified in a non-directory operation' status
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: a directory '%s' was specified for 'read' which is a non-directory operation\n", file_absolute_path);

        // build the procedure results
        Nfs__ReadRes *readres = create_default_case_read_res(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t readres_size = nfs__read_res__get_packed_size(readres);
        uint8_t *readres_buffer = malloc(readres_size);
        nfs__read_res__pack(readres, readres_buffer);

        nfs__read_args__free_unpacked(readargs, NULL);
        free(readres->default_case);
        free(readres);

        return wrap_procedure_results_in_successful_accepted_reply(readres_size, readres_buffer, "nfs/ReadRes");
    }
    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    // read from the file
    uint8_t *read_data = malloc(sizeof(uint8_t) * readargs->count);
    size_t bytes_read;
    error_code = read_from_file(file_absolute_path, readargs->offset, readargs->count, read_data, &bytes_read);
    if(error_code > 0) {
        // we failed to read from this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: Failed to read from file at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }

    // get the attributes of the looked up file after the read - this is what we return to client
    Nfs__FAttr fattr_after_read = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr_after_read);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__ReadRes readres = NFS__READ_RES__INIT;
    readres.status = NFS__STAT__NFS_OK;
    readres.body_case = NFS__READ_RES__BODY_READRESBODY;

    Nfs__ReadResBody readresbody = NFS__READ_RES_BODY__INIT;
    readresbody.attributes = &fattr_after_read;
    readresbody.nfsdata.data = read_data;
    readresbody.nfsdata.len = bytes_read;

    readres.readresbody = &readresbody;

    // serialize the procedure results
    size_t readres_size = nfs__read_res__get_packed_size(&readres);
    uint8_t *readres_buffer = malloc(readres_size);
    nfs__read_res__pack(&readres, readres_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(readres_size, readres_buffer, "nfs/ReadRes");

    nfs__read_args__free_unpacked(readargs, NULL);

    free(fattr_after_read.atime);
    free(fattr_after_read.mtime);
    free(fattr_after_read.ctime);

    return accepted_reply;
}

/*
* Runs the NFSPROC_WRITE procedure (8).
*/
Rpc__AcceptedReply serve_nfs_procedure_8_write_to_file(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/WriteArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: Expected nfs/WriteArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__WriteArgs *writeargs = nfs__write_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(writeargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: Failed to unpack WriteArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(writeargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: 'file' in WriteArgs is NULL \n");

        nfs__write_args__free_unpacked(writeargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *file_fhandle = writeargs->file;
    if(file_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: FHandle->nfs_filehandle is null\n");

        nfs__write_args__free_unpacked(writeargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(writeargs->nfsdata.data == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: nfsdata.data is null\n");

        nfs__write_args__free_unpacked(writeargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *file_nfs_filehandle = file_fhandle->nfs_filehandle;
    ino_t inode_number = file_nfs_filehandle->inode_number;

    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file - we assume the client gave us a wrong NFS filehandle, i.e. no such file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed to decode inode number %ld back to a file\n", inode_number);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    // get the attributes of the looked up file before the write, to check that the file is not a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // all file types except for directories can be written to as files
    if(fattr.type == NFS__FTYPE__NFDIR) {
        // if the file is actually directory, return AttrStat with 'directory specified in a non-directory operation' status
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: a directory '%s' was specified for 'write' which is a non-directory operation\n", file_absolute_path);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }
    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    // check if client requested to write too much data in a single RPC
    if(writeargs->nfsdata.len > NFS_MAXDATA) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: 'write' of %ld bytes attempted to file at absolute path '%s', but max write allowed is %d bytes\n", writeargs->nfsdata.len, file_absolute_path, NFS_MAXDATA);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_FBIG);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    // write to the file
    error_code = write_to_file(file_absolute_path, writeargs->offset, writeargs->nfsdata.len, writeargs->nfsdata.data);
    if(error_code == 4 || error_code == 5 || error_code == 6) {
        Nfs__Stat nfs_stat;
        switch(error_code) {
            case 4:
                nfs_stat = NFS__STAT__NFSERR_FBIG;
            case 5:
                nfs_stat = NFS__STAT__NFSERR_IO;
            case 6:
                nfs_stat = NFS__STAT__NFSERR_NOSPC;
        }

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(nfs_stat);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }
    if(error_code > 0) {
        // we failed writing to this file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed writing to file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        return create_system_error_accepted_reply();
    }

    // get the attributes of the file after the write - this is what we return to client
    Nfs__FAttr fattr_after_write = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr_after_write);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;
    attr_stat.status = NFS__STAT__NFS_OK;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;
    attr_stat.attributes = &fattr_after_write;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

    nfs__write_args__free_unpacked(writeargs, NULL);

    free(fattr_after_write.atime);
    free(fattr_after_write.mtime);
    free(fattr_after_write.ctime);

    return accepted_reply;
}

/*
* Runs the NFSPROC_READDIR procedure (16).
*/
Rpc__AcceptedReply serve_nfs_procedure_16_read_from_directory(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/ReadDirArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: Expected nfs/ReadDirArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__ReadDirArgs *readdirargs = nfs__read_dir_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(readdirargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: Failed to unpack ReadDirArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(readdirargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: 'dir' in ReadDirArgs is NULL \n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(readdirargs->cookie == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: 'cookie' in ReadDirArgs is NULL \n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = readdirargs->dir;
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: FHandle->nfs_filehandle is null\n");

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__ReadDirRes *readdirres = create_default_case_read_dir_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t readdirres_size = nfs__read_dir_res__get_packed_size(readdirres);
        uint8_t *readdirres_buffer = malloc(readdirres_size);
        nfs__read_dir_res__pack(readdirres, readdirres_buffer);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);
        free(readdirres->default_case);
        free(readdirres);

        return wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");
    }

    // get the attributes of this directory before the read, to check that it is actually a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed getting file attributes for file at absolute path '%s' with error code %d \n", directory_absolute_path, error_code);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    // only directories can be read using READDIR
    if(fattr.type != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return ReadDirRes with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: a non-directory '%s' was specified for 'readdir' which is a directory operation\n", directory_absolute_path);

        // build the procedure results
        Nfs__ReadDirRes *readdirres = create_default_case_read_dir_res(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t readdirres_size = nfs__read_dir_res__get_packed_size(readdirres);
        uint8_t *readdirres_buffer = malloc(readdirres_size);
        nfs__read_dir_res__pack(readdirres, readdirres_buffer);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);
        free(readdirres->default_case);
        free(readdirres);

        return wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");
    }
    free(fattr.atime);
    free(fattr.mtime);
    free(fattr.ctime);

    // read entries from the directory
    Nfs__DirectoryEntriesList *directory_entries = NULL;
    int end_of_stream = 0;
    error_code = read_from_directory(directory_absolute_path, readdirargs->cookie->value, readdirargs->count, &directory_entries, &end_of_stream);
    if(error_code > 0) {
        // we failed reading directory entries
        fprintf(stderr, "serve_nfs_procedure_16_read_from_directory: failed reading directory entries for directory at absolute path '%s' with error code %d \n", directory_absolute_path, error_code);

        nfs__read_dir_args__free_unpacked(readdirargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__ReadDirRes readdirres = NFS__READ_DIR_RES__INIT;
    readdirres.status = NFS__STAT__NFS_OK;
    readdirres.body_case = NFS__READ_DIR_RES__BODY_READDIROK;

    Nfs__ReadDirOk readdirok = NFS__READ_DIR_OK__INIT;
    readdirok.entries = directory_entries;
    readdirok.eof = end_of_stream;

    readdirres.readdirok = &readdirok;

    // serialize the procedure results
    size_t readdirres_size = nfs__read_dir_res__get_packed_size(&readdirres);
    uint8_t *readdirres_buffer = malloc(readdirres_size);
    nfs__read_dir_res__pack(&readdirres, readdirres_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(readdirres_size, readdirres_buffer, "nfs/ReadDirRes");

    nfs__read_dir_args__free_unpacked(readdirargs, NULL);

    clean_up_directory_entries_list(directory_entries);

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
        case 6:
            return serve_nfs_procedure_6_read_from_file(parameters);
        case 7:
        case 8:
            return serve_nfs_procedure_8_write_to_file(parameters);
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            return serve_nfs_procedure_16_read_from_directory(parameters);
        case 17:
        default:
    }

    // procedure not found
    Rpc__AcceptedReply accepted_reply = RPC__ACCEPTED_REPLY__INIT;
    accepted_reply.stat = RPC__ACCEPT_STAT__PROC_UNAVAIL;
    accepted_reply.reply_data_case = RPC__ACCEPTED_REPLY__REPLY_DATA_DEFAULT_CASE;
    return accepted_reply;
}