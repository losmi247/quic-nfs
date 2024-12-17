#include "nfsproc.h"

/*
* Runs the NFSPROC_SYMLINK procedure (13).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_13_create_symbolic_link(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/SymLinkArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: expected nfs/SymLinkArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__SymLinkArgs *symlinkargs = nfs__sym_link_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(symlinkargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: failed to unpack SymLinkArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(symlinkargs->from == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: 'from' in SymLinkArgs is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__DirOpArgs *from = symlinkargs->from;
    if(from->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: DirOpArgs->dir is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = from->dir;
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: FHandle->nfs_filehandle is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(from->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: DirOpArgs->name is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = from->name;
    if(file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: DirOpArgs->name->filename is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(symlinkargs->to == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: 'to' in SymLinkArgs is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__Path *to = symlinkargs->to;
    if(to->path == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: Path->path is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(symlinkargs->attributes == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: 'attributes' in SymLinkArgs is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__SAttr *sattr = symlinkargs->attributes;
    if(sattr->atime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: SAttr->atime is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(sattr->mtime == NULL) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: SAttr->mtime is null\n");

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *directory_nfs_filehandle = directory_fhandle->nfs_filehandle;
    ino_t inode_number = directory_nfs_filehandle->inode_number;

    char *directory_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(directory_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // get the attributes of this directory, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: failed getting file/directory attributes for file/directory at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if(directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        // if the file is not a directory, return NOTDIR NfsStat
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: 'symlink' procedure called on a non-directory '%s'\n", directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        clean_up_fattr(&directory_fattr);
        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    clean_up_fattr(&directory_fattr);

    // check if the name of the file to be created is longer than NFS limit
    if(strlen(file_name->filename) > NFS_MAXNAMLEN) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: attempted to create a symbolic link in directory '%s' with file name longer than NFS limit\n", directory_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_NAMETOOLONG);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }

    // check if the file client wants to create already exists
    char *file_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    error_code = access(file_absolute_path, F_OK);
    if(error_code == 0) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: attempted to create a symbolic link with at '%s' but file at that absolute path already exists\n", file_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_EXIST);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        free(file_absolute_path);
        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    else if(errno == EIO) {
        fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: physical IO error occurred while checking if file at absolute path '%s' exists\n", file_absolute_path);

        // build the procedure results
        Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_IO);

        // serialize the procedure results
        size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
        uint8_t *nfsstat_buffer = malloc(nfsstat_size);
        nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

        free(file_absolute_path);
        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
        free(nfs_status);

        return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
    }
    else if(errno != ENOENT) {
        // we got an error different from 'ENOENT = No such file or directory'
        perror_msg("serve_nfs_procedure_13_create_symbolic_link: failed checking if symbolic link to be created at absolute path '%s' is a file that already exists", file_absolute_path);

        free(file_absolute_path);
        nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

        return create_system_error_accepted_reply();
    }
    // now we know we got a ENOENT from access() i.e. the file client wants to create does not exist

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_symlink_proc_permissions(directory_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: failed checking SYMLINK permissions for creating a symbolic link at absolute path '%s' to target '%s' with error code %d\n", file_absolute_path, to->path, stat);

            free(file_absolute_path);
            nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to create this symbolic link
        if(stat == 1) {
            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // create the symbolic link
    error_code = symlink(to->path, file_absolute_path);
    if(error_code < 0) {
        if(errno == EIO || errno == ENAMETOOLONG || errno == ENOENT || errno == ENOSPC || errno == EROFS) {
            Nfs__Stat nfs_stat;
            switch(errno) {
                case EIO:
                    nfs_stat = NFS__STAT__NFSERR_IO;
                    fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: physical IO error occurred while trying to create a symbolic link at absolute path '%s' to target '%s'\n", file_absolute_path, to->path);
                    break;
                case ENAMETOOLONG:
                    nfs_stat = NFS__STAT__NFSERR_NAMETOOLONG;
                    fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: attempted to create a symbolic link at absolute path '%s' to target '%s', and one of these pathnames exceeds system limit on pathname length\n", file_absolute_path, to->path);
                    break;
                case ENOENT:
                    nfs_stat = NFS__STAT__NFSERR_NOENT;
                    fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: attempted to create a symbolic link at absolute path '%s' to target '%s', and some directory component in '%s' does not exist or is a dangling symbolic link\n", file_absolute_path, to->path, file_absolute_path);
                    break;
                case ENOSPC: 
                    nfs_stat = NFS__STAT__NFSERR_NOSPC;
                    fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: no space left to add a new entry '%s' to directory '%s'\n", file_absolute_path, directory_absolute_path);
                    break;
                case EROFS:
                    nfs_stat = NFS__STAT__NFSERR_ROFS;
                    fprintf(stderr, "serve_nfs_procedure_13_create_symbolic_link: attempted to create a symbolic link at '%s' on a read-only filesystem\n", file_absolute_path);
                    break;
            }

            // build the procedure results
            Nfs__NfsStat *nfs_status = create_nfs_stat(nfs_stat);

            // serialize the procedure results
            size_t nfsstat_size = nfs__nfs_stat__get_packed_size(nfs_status);
            uint8_t *nfsstat_buffer = malloc(nfsstat_size);
            nfs__nfs_stat__pack(nfs_status, nfsstat_buffer);

            free(file_absolute_path);
            nfs__sym_link_args__free_unpacked(symlinkargs, NULL);
            free(nfs_status);

            return wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");
        }
        else{
            perror_msg("serve_nfs_procedure_13_create_symbolic_link: failed creating symbolic link at absolute path '%s' to target '%s'\n", file_absolute_path, to->path);

            free(file_absolute_path);
            nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

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

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(nfsstat_size, nfsstat_buffer, "nfs/NfsStat");

    free(file_absolute_path);
    nfs__sym_link_args__free_unpacked(symlinkargs, NULL);

    return accepted_reply;
}