#include "nfsproc.h"

/*
* Runs the NFSPROC_SETATTR procedure (2).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_2_set_file_attributes(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/SAttrArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: expected nfs/SAttrArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__SAttrArgs *sattrargs = nfs__sattr_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(sattrargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: failed to unpack SAttrArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(sattrargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'file' in SAttrArgs is null\n");

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
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'attributes' in SAttrArgs is null\n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__SAttr *sattr = sattrargs->attributes;
    if(sattr->atime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'atime' in SAttrArgs is null\n");

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(sattr->mtime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: 'mtime' in SAttrArgs is null\n");

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
        free(attr_stat->nfs_status);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_setattr_proc_permissions(file_absolute_path, sattr, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_2_set_file_attributes: failed checking SETATTR permissions for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, stat);

            nfs__fhandle__free_unpacked(fhandle, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to set attributes of this file/directory
        if(stat == 1) {
            // build the procedure results
            Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
            uint8_t *attr_stat_buffer = malloc(attr_stat_size);
            nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

            nfs__sattr_args__free_unpacked(sattrargs, NULL);
            free(attr_stat->nfs_status);
            free(attr_stat->default_case);
            free(attr_stat);

            return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // update file attributes - -1 means don't update this attribute
    // TODO (QNFS-21) - change this to either update all arguments or none (don't want a partial update)
    if(sattr->mode != -1 && chmod(file_absolute_path, sattr->mode) < 0) {
        perror_msg("serve_nfs_procedure_2_set_file_attributes: failed to update 'mode' attribute of file/directory at absolute path '%s'\n", file_absolute_path);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(chown(file_absolute_path, sattr->uid, sattr->gid) < 0) { // don't need to check if uid/gid is -1, as chown ignores uid or gid if it's -1
        perror_msg("serve_nfs_procedure_2_set_file_attributes: failed to update 'uid' and 'gid' attributes of file/directory at absolute path '%s'\n", file_absolute_path);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        return create_system_error_accepted_reply();
    }
    if(sattr->size != -1 && truncate(file_absolute_path, sattr->size) < 0) {
        perror_msg("serve_nfs_procedure_2_set_file_attributes: failed to update 'size' attributes of file/directory at absolute path '%s'\n", file_absolute_path);

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
            perror_msg("serve_nfs_procedure_2_set_file_attributes: failed to update 'atime' and 'mtime' attributes of file/directory at absolute path '%s'\n", file_absolute_path);

            nfs__sattr_args__free_unpacked(sattrargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_procedure_2_set_file_attributes: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__sattr_args__free_unpacked(sattrargs, NULL);

        // we return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded inode number to a file
        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;

    attr_stat.nfs_status = &nfs_status;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;
    attr_stat.attributes = &fattr;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

    nfs__sattr_args__free_unpacked(sattrargs, NULL);
    clean_up_fattr(&fattr);

    return accepted_reply;
}