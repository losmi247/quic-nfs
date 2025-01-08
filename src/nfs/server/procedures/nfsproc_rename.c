#include "nfsproc.h"

/*
* Runs the NFSPROC_RENAME procedure (11).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_11_rename_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/RenameArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: expected nfs/RenameArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__RenameArgs *renameargs = nfs__rename_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(renameargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed to unpack RenameArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(renameargs->from == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: 'from' in RenameArgs is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);
        
        return create_garbage_args_accepted_reply();
    }
    Nfs__DirOpArgs *from = renameargs->from;
    if(from->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: from->dir is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *from_directory_fhandle = from->dir;
    if(from_directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: FHandle->nfs_filehandle is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(from->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: from->name is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *from_file_name = from->name;
    if(from_file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: FileName->filename is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(renameargs->to == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: 'to' in RenameArgs is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);
        
        return create_garbage_args_accepted_reply();
    }
    Nfs__DirOpArgs *to = renameargs->to;
    if(to->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: to->dir is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *to_directory_fhandle = to->dir;
    if(to_directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: FHandle->nfs_filehandle is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(to->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: to->name is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *to_file_name = to->name;
    if(to_file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: FileName->filename is null\n");

        nfs__rename_args__free_unpacked(renameargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *from_directory_nfs_filehandle = from_directory_fhandle->nfs_filehandle;
    ino_t from_dir_inode_number = from_directory_nfs_filehandle->inode_number;

    char *from_directory_absolute_path = get_absolute_path_from_inode_number(from_dir_inode_number, inode_cache);
    if(from_directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed to decode inode number %ld back to a directory\n", from_dir_inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__rename_args__free_unpacked(renameargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of the 'from' directory, to check that it is actually a directory
    Nfs__FAttr from_directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(from_directory_absolute_path, &from_directory_fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed getting file/directory attributes for file/directory at absolute path '%s' with error code %d\n", from_directory_absolute_path, error_code);

        nfs__rename_args__free_unpacked(renameargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if(from_directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return NfsStat with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: 'rename' procedure called with 'from' being a non-directory '%s'\n", from_directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&from_directory_fattr);
        nfs__rename_args__free_unpacked(renameargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&from_directory_fattr);

    // check that the file/directory client wants to rename exists
    char *old_file_absolute_path = get_file_absolute_path(from_directory_absolute_path, from_file_name->filename);
    struct stat file_stat;
    error_code = lstat(old_file_absolute_path, &file_stat);
    if(error_code < 0) {
        if(errno == ENOENT) {
            Nfs__Stat nfs_stat;
            switch(errno) {
                case ENOENT:
                    nfs_stat = NFS__STAT__NFSERR_NOENT;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: attempted to rename a file '%s' that does not exist\n", old_file_absolute_path);
                    break;
            }

            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(nfs_stat);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(old_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
        else {
            perror_msg("serve_nfs_procedure_11_rename_file: failed checking if file to be renamed at absolute path '%s' already exists", old_file_absolute_path);

            free(old_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    NfsFh__NfsFileHandle *to_directory_nfs_filehandle = to_directory_fhandle->nfs_filehandle;
    ino_t to_dir_inode_number = to_directory_nfs_filehandle->inode_number;

    char *to_directory_absolute_path = get_absolute_path_from_inode_number(to_dir_inode_number, inode_cache);
    if(to_directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed to decode inode number %ld back to a directory\n", to_dir_inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        free(old_file_absolute_path);
        nfs__rename_args__free_unpacked(renameargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of the 'to' directory, to check that it is actually a directory
    Nfs__FAttr to_directory_fattr = NFS__FATTR__INIT;
    error_code = get_attributes(to_directory_absolute_path, &to_directory_fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed getting file/directory attributes for file/directory at absolute path '%s' with error code %d\n", to_directory_absolute_path, error_code);

        free(old_file_absolute_path);
        nfs__rename_args__free_unpacked(renameargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if(to_directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return NfsStat with 'non-directory specified in a directory operation' status
        fprintf(stderr, "serve_nfs_procedure_11_rename_file: 'rename' procedure called with 'to' being a non-directory '%s'\n", to_directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&to_directory_fattr);
        free(old_file_absolute_path);
        nfs__rename_args__free_unpacked(renameargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&to_directory_fattr);

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_rename_proc_permissions(from_directory_absolute_path, to_directory_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_11_rename_file: failed checking RENAME permissions for moving a file at absolute path '%s' to '%s/%s' with error code %d\n", old_file_absolute_path, to_directory_absolute_path, to_file_name->filename, stat);

            free(old_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to move file
        if(stat == 1) {
            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(old_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // rename the file/directory
    char *new_file_absolute_path = get_file_absolute_path(to_directory_absolute_path, to_file_name->filename);
    error_code = rename(old_file_absolute_path, new_file_absolute_path);
    if(error_code < 0) {
        if(errno == EDQUOT || errno == EINVAL || errno == ENAMETOOLONG || errno == ENOENT || errno == ENOSPC || errno == ENOTEMPTY || errno == EROFS) {
            Nfs__Stat nfs_stat;
            switch(errno) {
                case EDQUOT:
                    nfs_stat = NFS__STAT__NFSERR_DQUOT;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: the user's quota of disk blocks on the file system has been exhausted when trying to rename file at absolute path '%s' to '%s'\n", old_file_absolute_path, new_file_absolute_path);
                    break;
                case EINVAL:
                    nfs_stat = NFS__STAT__NFSERR_EXIST;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: an attempt was made to make a directory a subdirectory of itself when trying to rename file at absolute path '%s' to '%s'\n", old_file_absolute_path, new_file_absolute_path);
                    break;
                case ENAMETOOLONG:
                    nfs_stat = NFS__STAT__NFSERR_NAMETOOLONG;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: attempted to rename a file at absolute path '%s' to '%s', one of which exceeds system limit on pathname length\n", old_file_absolute_path, new_file_absolute_path);
                    break;
                case ENOENT:
                    nfs_stat = NFS__STAT__NFSERR_NOENT;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: a directory component in new absolute path '%s' does not exist\n", new_file_absolute_path);
                    break;
                case ENOSPC:
                    nfs_stat = NFS__STAT__NFSERR_NOSPC;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: the device containing the file has no room for the new directory entry\n");
                    break;
                case ENOTEMPTY:
                    nfs_stat = NFS__STAT__NFSERR_NOTEMPTY;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: attempted to move file at absolute path '%s' to absolute path '%s' which is a nonempty directory\n", old_file_absolute_path, new_file_absolute_path); // not allowed to overwrite nonempty directories!
                    break;
                case EROFS:
                    nfs_stat = NFS__STAT__NFSERR_ROFS;
                    fprintf(stderr, "serve_nfs_procedure_11_rename_file: attempted to rename a file at absolute path '%s' to '%s' on a read-only file system\n", old_file_absolute_path, new_file_absolute_path);
                    break;
            }

            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(nfs_stat);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(old_file_absolute_path);
            free(new_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
        else {
            perror_msg("serve_nfs_procedure_11_rename_file: failed to 'rename' the file at absolute path '%s' to absolute path\n", old_file_absolute_path, new_file_absolute_path);

            free(old_file_absolute_path);
            free(new_file_absolute_path);
            nfs__rename_args__free_unpacked(renameargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // remove the inode mapping for the old absolute path in the inode cache
    error_code = update_inode_mapping_absolute_path_by_absolute_path(old_file_absolute_path, new_file_absolute_path, &inode_cache);
    if(error_code > 1) {
        perror_msg("serve_nfs_procedure_11_rename_file: failed to update the inode mapping for file at absolute path '%s' being moved to absolute path '%s'\n", old_file_absolute_path, new_file_absolute_path);

        free(old_file_absolute_path);
        free(new_file_absolute_path);
        nfs__rename_args__free_unpacked(renameargs, NULL);

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

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");

    free(old_file_absolute_path);
    free(new_file_absolute_path);
    nfs__rename_args__free_unpacked(renameargs, NULL);

    return accepted_reply;
}