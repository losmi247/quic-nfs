#include "nfsproc.h"

/*
 * Runs the NFSPROC_LINK procedure (12).
 *
 * Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
 * credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported
 * authentication flavor) before being passed here. This procedure must not be given AUTH_NONE credential+verifier pair.
 *
 * The user of this function takes the responsibility to deallocate the received AcceptedReply
 * using the 'free_accepted_reply()' function.
 */
Rpc__AcceptedReply *serve_nfs_procedure_12_create_link_to_file(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier,
                                                               Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if (parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/LinkArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: expected nfs/LinkArgs but received %s\n",
                parameters->type_url);

        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__LinkArgs *linkargs = nfs__link_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if (linkargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: failed to unpack LinkArgs\n");

        return create_garbage_args_accepted_reply();
    }
    if (linkargs->from == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: 'from' in LinkArgs is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *target_file_fhandle = linkargs->from;
    if (target_file_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: FHandle->nfs_filehandle is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if (linkargs->to == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: 'to' in LinkArgs is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__DirOpArgs *to = linkargs->to;
    if (to->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: DirOpArgs->dir is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = to->dir;
    if (directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: FHandle->nfs_filehandle is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if (to->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: DirOpArgs->name is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = to->name;
    if (file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: DirOpArgs->name->filename is null\n");

        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *target_file_nfs_filehandle = target_file_fhandle->nfs_filehandle;
    ino_t target_file_inode_number = target_file_nfs_filehandle->inode_number;

    char *target_file_absolute_path = get_absolute_path_from_inode_number(target_file_inode_number, inode_cache);
    if (target_file_absolute_path == NULL) {
        // we couldn't decode inode number back to a file - we assume the client gave us a wrong NFS filehandle, i.e. no
        // such file
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: failed to decode inode number %ld back to a file\n",
                target_file_inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of the looked up file before creating a hard link to it, to check that the file is a regular
    // file
    Nfs__FAttr target_file_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(target_file_absolute_path, &target_file_fattr);
    if (error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: failed getting attributes for file/directory at absolute "
                "path '%s' with error code %d\n",
                target_file_absolute_path, error_code);

        nfs__link_args__free_unpacked(linkargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this
        // file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // we can only make hard links to regular files
    if (target_file_fattr.nfs_ftype->ftype != NFS__FTYPE__NFREG) {
        // if the file is actually directory, return 'directory specified in a non-directory operation' status
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: file '%s' which is not a regular file was specified for "
                "'link' which is only possible for regular files\n",
                target_file_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&target_file_fattr);
        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&target_file_fattr);

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if (directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS
        // filehandle, i.e. no such directory
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: failed to decode inode number %ld back to a directory\n",
                inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of this directory, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if (error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: failed getting file/directory attributes for "
                "file/directory at absolute path '%s' with error code %d\n",
                directory_absolute_path, error_code);

        nfs__link_args__free_unpacked(linkargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this
        // directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if (directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return NOTDIR NfsStat
        fprintf(stderr, "serve_nfs_procedure_12_create_link_to_file: 'link' procedure called on a non-directory '%s'\n",
                directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&directory_fattr);
        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&directory_fattr);

    // check if the name of the file to be created is longer than NFS limit
    if (strlen(file_name->filename) > NFS_MAXNAMLEN) {
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: attempted to create a hard link in directory '%s' with "
                "file name longer than NFS limit\n",
                directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NAMETOOLONG);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // check if the file client wants to create already exists
    char *file_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    struct stat file_stat;
    error_code = lstat(file_absolute_path, &file_stat);
    if (error_code == 0) {
        fprintf(stderr,
                "serve_nfs_procedure_12_create_link_to_file: attempted to create a hard link with at '%s' but file at "
                "that absolute path already exists\n",
                file_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_EXIST);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        free(file_absolute_path);
        nfs__link_args__free_unpacked(linkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    } else if (errno != ENOENT) {
        // we got an error different from 'ENOENT = No such file or directory'
        perror_msg("serve_nfs_procedure_12_create_link_to_file: failed checking if hard link to be created at absolute "
                   "path '%s' is a file that already exists",
                   file_absolute_path);

        free(file_absolute_path);
        nfs__link_args__free_unpacked(linkargs, NULL);

        return create_system_error_accepted_reply();
    }
    // now we know we got a ENOENT from lstat() i.e. the file client wants to create does not exist

    // check permissions
    if (credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat =
            check_link_proc_permissions(directory_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if (stat < 0) {
            fprintf(stderr,
                    "serve_nfs_procedure_12_create_link_to_file: failed checking LINK permissions for creating a hard "
                    "link at absolute path '%s' to target '%s' with error code %d\n",
                    file_absolute_path, target_file_absolute_path, stat);

            free(file_absolute_path);
            nfs__link_args__free_unpacked(linkargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to create this hard link
        if (stat == 1) {
            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__link_args__free_unpacked(linkargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with
    // supported authentication flavor)

    // create the hard link
    error_code = link(target_file_absolute_path, file_absolute_path);
    if (error_code < 0) {
        if (errno == EDQUOT || errno == EIO || errno == ENAMETOOLONG || errno == ENOENT || errno == ENOSPC ||
            errno == EROFS) {
            Nfs__Stat nfs_stat;
            switch (errno) {
            case EDQUOT:
                nfs_stat = NFS__STAT__NFSERR_DQUOT;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: user's quota of disk blocks on the filesystem has "
                        "been exhausted while trying to create a hard link at absolute path '%s' to target '%s'\n",
                        file_absolute_path, target_file_absolute_path);
                break;
            case EIO:
                nfs_stat = NFS__STAT__NFSERR_IO;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: physical IO error occurred while trying to create "
                        "a hard link at absolute path '%s' to target '%s'\n",
                        file_absolute_path, target_file_absolute_path);
                break;
            case ENAMETOOLONG:
                nfs_stat = NFS__STAT__NFSERR_NAMETOOLONG;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: attempted to create a hard link at absolute path "
                        "'%s' to target '%s', and one of these pathnames exceeds system limit on pathname length\n",
                        file_absolute_path, target_file_absolute_path);
                break;
            case ENOENT:
                nfs_stat = NFS__STAT__NFSERR_NOENT;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: attempted to create a hard link at absolute path "
                        "'%s' to target '%s', and some directory component in these two paths does not exist or is a "
                        "dangling symbolic link\n",
                        file_absolute_path, target_file_absolute_path);
                break;
            case ENOSPC:
                nfs_stat = NFS__STAT__NFSERR_NOSPC;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: no space left to add a new entry '%s' to "
                        "directory '%s'\n",
                        file_absolute_path, directory_absolute_path);
                break;
            case EROFS:
                nfs_stat = NFS__STAT__NFSERR_ROFS;
                fprintf(stderr,
                        "serve_nfs_procedure_12_create_link_to_file: attempted to create a hard link at '%s' on a "
                        "read-only filesystem\n",
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
            nfs__link_args__free_unpacked(linkargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        } else {
            perror_msg("serve_nfs_procedure_12_create_link_to_file: failed creating hard link at absolute path '%s' to "
                       "target '%s'\n",
                       file_absolute_path, target_file_absolute_path);

            free(file_absolute_path);
            nfs__link_args__free_unpacked(linkargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

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
    nfs__link_args__free_unpacked(linkargs, NULL);

    return accepted_reply;
}