#include "nfsproc.h"

/*
* Runs the NFSPROC_MKDIR procedure (14).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_14_create_directory(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/CreateArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: expected nfs/CreateArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__CreateArgs *createargs = nfs__create_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(createargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed to unpack CreateArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(createargs->where == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: 'where' in CreateArgs is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__DirOpArgs *diropargs = createargs->where;
    if(diropargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: DirOpArgs->dir is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = diropargs->dir;
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: FHandle->nfs_filehandle is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(diropargs->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: DirOpArgs->name is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = diropargs->name;
    if(file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: DirOpArgs->name->filename is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(createargs->attributes == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: 'attributes' in CreateArgs is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__SAttr *sattr = createargs->attributes;
    if(sattr->atime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: SAttr->atime is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(sattr->mtime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: SAttr->mtime is null\n");

        nfs__create_args__free_unpacked(createargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        nfs__create_args__free_unpacked(createargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }

    // get the attributes of this directory, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed getting file/directory attributes for file/directory at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        nfs__create_args__free_unpacked(createargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if(directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return DirOpRes with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: 'mkdir' procedure called on a non-directory '%s'\n", directory_absolute_path);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        clean_up_fattr(&directory_fattr);
        nfs__create_args__free_unpacked(createargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }
    clean_up_fattr(&directory_fattr);

    // check if the name of the directory to be created is longer than NFS limit
    if(strlen(file_name->filename) > NFS_MAXNAMLEN) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: attempted to create directory in directory '%s' with file name longer than NFS limit\n", directory_absolute_path);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NAMETOOLONG);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        nfs__create_args__free_unpacked(createargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }

    // check if the directory client wants to create already exists
    char *child_directory_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    struct stat directory_stat;
    error_code = lstat(child_directory_absolute_path, &directory_stat);
    if(error_code == 0) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: attempted to create a directory '%s' that already exists\n", child_directory_absolute_path);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_EXIST);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }
    else if(errno != ENOENT) {
        // we got an error different from 'ENOENT = No such file or directory'
        perror_msg("serve_nfs_procedure_14_create_directory: failed checking if directory to be created at absolute path '%s' already exists", child_directory_absolute_path);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);

        return create_system_error_accepted_reply();
    }
    // now we know we got a ENOENT from lstat() i.e. the directory client wants to create does not exist

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_mkdir_proc_permissions(directory_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed checking MKDIR permissions for creating a directory at absolute path '%s' with error code %d\n", child_directory_absolute_path, stat);

            free(child_directory_absolute_path);
            nfs__create_args__free_unpacked(createargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to create a directory here
        if(stat == 1) {
            // build the procedure results
            Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
            uint8_t *diropres_buffer = malloc(diropres_size);
            nfs__dir_op_res__pack(diropres, diropres_buffer);

            free(child_directory_absolute_path);
            nfs__create_args__free_unpacked(createargs, NULL);
            free(diropres->nfs_status);
            free(diropres->default_case);
            free(diropres);

            return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // create the directory
    if(sattr->mode != -1) {
        error_code = mkdir(child_directory_absolute_path, sattr->mode);
    }
    else{
        error_code = mkdir(child_directory_absolute_path, 0700);
    }
    if(error_code < 0) {
        if(errno == EDQUOT || errno == ENAMETOOLONG || errno == ENOSPC || errno == EROFS) {
            Nfs__Stat nfs_stat;
            switch(errno) {
                case EDQUOT:
                    nfs_stat = NFS__STAT__NFSERR_DQUOT;
                    fprintf(stderr, "serve_nfs_procedure_14_create_directory: tried to create directory at absolute path '%s', but user's quota of disk blocks or inodes on the file system has been exhausted\n", child_directory_absolute_path);
                    break;
                case ENAMETOOLONG:
                    nfs_stat = NFS__STAT__NFSERR_NAMETOOLONG;
                    fprintf(stderr, "serve_nfs_procedure_14_create_directory: attempted to create a directory at absolute path '%s' which exceeds system limit on pathname length\n", child_directory_absolute_path);
                    break;
                case ENOSPC: 
                    nfs_stat = NFS__STAT__NFSERR_NOSPC;
                    fprintf(stderr, "serve_nfs_procedure_14_create_directory: no room for directory '%s' to be created on this device\n", child_directory_absolute_path);
                    break;
                case EROFS:
                    nfs_stat = NFS__STAT__NFSERR_ROFS;
                    fprintf(stderr, "serve_nfs_procedure_14_create_directory: attempted to create a directory at absolute path '%s' on a read-only file system\n", child_directory_absolute_path);
                    break;
            }

            // build the procedure results
            Nfs__DirOpRes *diropres = create_default_case_dir_op_res(nfs_stat);

            // serialize the procedure results
            size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
            uint8_t *diropres_buffer = malloc(diropres_size);
            nfs__dir_op_res__pack(diropres, diropres_buffer);

            free(child_directory_absolute_path);
            nfs__create_args__free_unpacked(createargs, NULL);
            free(diropres->nfs_status);
            free(diropres->default_case);
            free(diropres);

            return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
        }
        else{
            perror_msg("serve_nfs_procedure_14_create_directory: failed creating directory at absolute path '%s'\n", child_directory_absolute_path);

            free(child_directory_absolute_path);
            nfs__create_args__free_unpacked(createargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // set initial attributes for the created directory
    if(chown(child_directory_absolute_path, sattr->uid, sattr->gid) < 0) { // don't need to check if uid/gid is -1, as chown ignores uid or gid if it's -1
        perror_msg("serve_nfs_procedure_14_create_directory: failed to initialize 'uid' and 'gid' attributes of the directory created at absolute path '%s'\n", child_directory_absolute_path);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(sattr->size != -1) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: can not set 'size' attribute of a directory\n");

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }
    if(sattr->atime->seconds != -1 && sattr->atime->useconds != -1 && sattr->mtime->seconds != -1 && sattr->mtime->useconds != -1) { // API only allows changing of both atime and mtime at once
        struct timeval times[2];
        times[0].tv_sec = sattr->atime->seconds;
        times[0].tv_usec = sattr->atime->useconds;
        times[1].tv_sec = sattr->mtime->seconds;
        times[1].tv_usec = sattr->mtime->useconds;

        if(utimes(child_directory_absolute_path, times) < 0) {
            perror_msg("serve_nfs_procedure_14_create_directory: failed to initialize 'atime' and 'mtime' attributes of the directory created at absolute path '%s'\n", child_directory_absolute_path);

            free(child_directory_absolute_path);
            nfs__create_args__free_unpacked(createargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // create a NFS filehandle for the created directory
    NfsFh__NfsFileHandle child_directory_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    error_code = create_nfs_filehandle(child_directory_absolute_path, &child_directory_nfs_filehandle, &inode_cache);
    if(error_code > 0) {
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed creating a NFS filehandle for directory at absolute path '%s' with error code %d\n", child_directory_absolute_path, error_code);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as once we've created the directory we have to be able to create a NFS filehandle for it
        return create_system_error_accepted_reply();
    }

    // get the attributes of the created directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    error_code = get_attributes(child_directory_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this directory
        fprintf(stderr, "serve_nfs_procedure_14_create_directory: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", child_directory_absolute_path, error_code);

        free(child_directory_absolute_path);
        nfs__create_args__free_unpacked(createargs, NULL);
        // remove the inode cache mapping for the created directory that we created when creating the NFS filehandle, as MKDIR was unsuccessful
        remove_inode_mapping_by_inode_number(child_directory_nfs_filehandle.inode_number, &inode_cache);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__DirOpRes diropres = NFS__DIR_OP_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;

    diropres.nfs_status = &nfs_status;
    diropres.body_case = NFS__DIR_OP_RES__BODY_DIROPOK;

    Nfs__FHandle child_directory_fhandle = NFS__FHANDLE__INIT;
    child_directory_fhandle.nfs_filehandle = &child_directory_nfs_filehandle;

    Nfs__DirOpOk diropok = NFS__DIR_OP_OK__INIT;
    diropok.file = &child_directory_fhandle;
    diropok.attributes = &fattr;

    diropres.diropok = &diropok;

    // serialize the procedure results
    size_t diropres_size = nfs__dir_op_res__get_packed_size(&diropres);
    uint8_t *diropres_buffer = malloc(diropres_size);
    nfs__dir_op_res__pack(&diropres, diropres_buffer);

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");

    nfs__create_args__free_unpacked(createargs, NULL);

    clean_up_fattr(&fattr);

    free(child_directory_absolute_path);

    return accepted_reply;
}