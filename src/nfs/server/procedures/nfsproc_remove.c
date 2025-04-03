#include "nfsproc.h"

/*
 * Runs the NFSPROC_REMOVE procedure (10).
 *
 * Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
 * credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported
 * authentication flavor) before being passed here. This procedure must not be given AUTH_NONE credential+verifier pair.
 *
 * The user of this function takes the responsibility to deallocate the received AcceptedReply
 * using the 'free_accepted_reply()' function.
 */
Rpc__AcceptedReply *serve_nfs_procedure_10_remove_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier,
                                                       Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if (parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/DirOpArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: expected nfs/DirOpArgs but received %s\n",
                parameters->type_url);

        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__DirOpArgs *diropargs = nfs__dir_op_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if (diropargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: failed to unpack DirOpArgs\n");

        return create_garbage_args_accepted_reply();
    }
    if (diropargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: 'dir' in DirOpArgs is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = diropargs->dir;
    if (directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: FHandle->nfs_filehandle is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if (diropargs->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: 'name' in DirOpArgs is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = diropargs->name;
    if (file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: FileName->filename is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if (directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS
        // filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: failed to decode inode number %ld back to a directory\n",
                inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of this directory, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if (error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr,
                "serve_nfs_procedure_10_remove_file: failed getting file/directory attributes for file/directory at "
                "absolute path '%s' with error code %d\n",
                directory_absolute_path, error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this
        // directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if (directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return NfsStat with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_10_remove_file: 'remove' procedure called on a non-directory '%s'\n",
                directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&directory_fattr);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&directory_fattr);

    // check that the file client wants to delete exists
    char *file_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    struct stat file_stat;
    error_code = lstat(file_absolute_path, &file_stat);
    if (error_code < 0) {
        if (errno == ENOENT) {
            Nfs__Stat nfs_stat;
            switch (errno) {
            case ENOENT:
                nfs_stat = NFS__STAT__NFSERR_NOENT;
                fprintf(stderr,
                        "serve_nfs_procedure_10_remove_file: attempted to delete a file '%s' that does not exist\n",
                        file_absolute_path);
                break;
            }

            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(nfs_stat);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        } else {
            perror_msg("serve_nfs_procedure_10_remove_file: failed checking if file to be deleted at absolute path "
                       "'%s' already exists",
                       file_absolute_path);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // get the attributes of the file to be deleted, to check that that it is not a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr);
    if (error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr,
                "serve_nfs_procedure_10_remove_file: failed getting attributes for file/directory at absolute path "
                "'%s' with error code %d\n",
                file_absolute_path, error_code);

        free(file_absolute_path);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this
        // file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // all file types except for directories can be deleted as files
    if (fattr.nfs_ftype->ftype == NFS__FTYPE__NFDIR) {
        // if the file is actually directory, return NfsStat with 'directory specified in a non-directory operation'
        // status
        fprintf(stderr,
                "serve_nfs_procedure_10_remove_file: a directory '%s' was specified for 'remove' which is a "
                "non-directory operation\n",
                file_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&fattr);
        free(file_absolute_path);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&fattr);

    // check permissions
    if (credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_remove_proc_permissions(directory_absolute_path, credential->auth_sys->uid,
                                                 credential->auth_sys->gid);
        if (stat < 0) {
            fprintf(stderr,
                    "serve_nfs_procedure_10_remove_file: failed checking REMOVE permissions for removing a file at "
                    "absolute path '%s' with error code %d\n",
                    file_absolute_path, stat);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to remove this file
        if (stat == 1) {
            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with
    // supported authentication flavor)

    // delete the file name
    error_code = unlink(file_absolute_path);
    if (error_code < 0) {
        if (errno == EIO || errno == ENAMETOOLONG || errno == EROFS) {
            Nfs__Stat nfs_stat;
            switch (errno) {
            case EIO:
                nfs_stat = NFS__STAT__NFSERR_IO;
                fprintf(stderr,
                        "serve_nfs_procedure_10_remove_file: physical IO error occurred while trying to delete a file "
                        "at absolute path '%s'\n",
                        file_absolute_path);
                break;
            case ENAMETOOLONG:
                nfs_stat = NFS__STAT__NFSERR_NAMETOOLONG;
                fprintf(stderr,
                        "serve_nfs_procedure_10_remove_file: attempted to delete a file at absolute path '%s' which "
                        "exceeds system limit on pathname length\n",
                        file_absolute_path);
                break;
            case EROFS:
                nfs_stat = NFS__STAT__NFSERR_ROFS;
                fprintf(stderr,
                        "serve_nfs_procedure_10_remove_file: attempted to delete a file at absolute path '%s' on a "
                        "read-only file system\n",
                        file_absolute_path);
                break;
            }

            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(nfs_stat);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        } else {
            perror_msg("serve_nfs_procedure_10_remove_file: failed to 'unlink' the file at absolute path '%s'\n",
                       file_absolute_path);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // remove the inode mapping of the deleted file from the inode cache
    error_code = remove_inode_mapping_by_absolute_path(file_absolute_path, &inode_cache);
    if (error_code > 1) {
        perror_msg(
            "serve_nfs_procedure_10_remove_file: failed to remove the inode mapping for file at absolute path '%s'\n",
            file_absolute_path);

        free(file_absolute_path);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_system_error_accepted_reply();
    }
    // here error_code = 0 or 1, so either the inode mapping was not found or was successfully removed

    // build the procedure results
    Nfs__NfsStat nfsstat = NFS__NFS_STAT__INIT;
    nfsstat.stat = NFS__STAT__NFS_OK;

    // serialize the procedure results
    size_t nfsstat_size = nfs__nfs_stat__get_packed_size(&nfsstat);
    uint8_t *nfsstat_buffer = malloc(nfsstat_size);
    nfs__nfs_stat__pack(&nfsstat, nfsstat_buffer);

    Rpc__AcceptedReply *accepted_reply =
        wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");

    free(file_absolute_path);
    nfs__dir_op_args__free_unpacked(diropargs, NULL);

    return accepted_reply;
}