#include "nfsproc.h"

/*
* Runs the NFSPROC_READLINK procedure (5).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_5_read_from_symbolic_link(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: expected nfs/FHandle but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__FHandle *symlink_fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(symlink_fhandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: failed to unpack FHandle\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(symlink_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: 'nfs_filehandle' in FHandle is null\n");

        nfs__fhandle__free_unpacked(symlink_fhandle, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *symlink_nfs_filehandle = symlink_fhandle->nfs_filehandle;
    ino_t inode_number = symlink_nfs_filehandle->inode_number;

    char *symlink_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(symlink_absolute_path == NULL) {
        // we couldn't decode inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such directory
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: failed to decode inode number %ld back to a directory\n", inode_number);

        // build the procedure results
        Nfs__ReadLinkRes *readlinkres = create_default_case_read_link_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t readlinkres_size = nfs__read_link_res__get_packed_size(readlinkres);
        uint8_t *readlinkres_buffer = malloc(readlinkres_size);
        nfs__read_link_res__pack(readlinkres, readlinkres_buffer);

        nfs__fhandle__free_unpacked(symlink_fhandle, NULL);
        free(readlinkres->nfs_status);
        free(readlinkres->default_case);
        free(readlinkres);

        return wrap_procedure_results_in_successful_accepted_reply(readlinkres_size, readlinkres_buffer, "nfs/ReadLinkRes");
    }

    // get the attributes of this directory, to check that it is actually a symbolic link
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(symlink_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: failed getting file/directory attributes for file/directory at absolute path '%s' with error code %d\n", symlink_absolute_path, error_code);

        nfs__fhandle__free_unpacked(symlink_fhandle, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    if(fattr.nfs_ftype->ftype != NFS__FTYPE__NFLNK) {
        // if the file is not a directory, return NOTDIR NfsStat
        fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: 'readlink' procedure called on a file '%s' which is not a symbolic link\n", symlink_absolute_path);

        // build the procedure results
        Nfs__ReadLinkRes *readlinkres = create_default_case_read_link_res(NFS__STAT__NFSERR_STALE);

        // serialize the procedure results
        size_t readlinkres_size = nfs__read_link_res__get_packed_size(readlinkres);
        uint8_t *readlinkres_buffer = malloc(readlinkres_size);
        nfs__read_link_res__pack(readlinkres, readlinkres_buffer);

        clean_up_fattr(&fattr);
        nfs__fhandle__free_unpacked(symlink_fhandle, NULL);
        free(readlinkres->nfs_status);
        free(readlinkres->default_case);
        free(readlinkres);

        return wrap_procedure_results_in_successful_accepted_reply(readlinkres_size, readlinkres_buffer, "nfs/ReadLinkRes");
    }
    clean_up_fattr(&fattr);

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_readlink_proc_permissions(symlink_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_5_read_from_symbolic_link: failed checking READLINK permissions for reading the path inside the symbolic link at absolute path '%s' with error code %d\n", symlink_absolute_path, stat);

            nfs__fhandle__free_unpacked(symlink_fhandle, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to read from this symbolic link
        if(stat == 1) {
            // build the procedure results
            Nfs__ReadLinkRes *readlinkres = create_default_case_read_link_res(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t readlinkres_size = nfs__read_link_res__get_packed_size(readlinkres);
            uint8_t *readlinkres_buffer = malloc(readlinkres_size);
            nfs__read_link_res__pack(readlinkres, readlinkres_buffer);

            nfs__fhandle__free_unpacked(symlink_fhandle, NULL);
            free(readlinkres->nfs_status);
            free(readlinkres->default_case);
            free(readlinkres);

            return wrap_procedure_results_in_successful_accepted_reply(readlinkres_size, readlinkres_buffer, "nfs/ReadLinkRes");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // read the path contained in the symbolic link
    char target_path[NFS_MAXPATHLEN + 1];
    int bytes_read = readlink(symlink_absolute_path, target_path, NFS_MAXPATHLEN);
    if(bytes_read < 0) {
        if(errno == EIO) {
            // build the procedure results
            Nfs__ReadLinkRes *readlinkres = create_default_case_read_link_res(NFS__STAT__NFSERR_IO);

            // serialize the procedure results
            size_t readlinkres_size = nfs__read_link_res__get_packed_size(readlinkres);
            uint8_t *readlinkres_buffer = malloc(readlinkres_size);
            nfs__read_link_res__pack(readlinkres, readlinkres_buffer);

            nfs__fhandle__free_unpacked(symlink_fhandle, NULL);
            free(readlinkres->nfs_status);
            free(readlinkres->default_case);
            free(readlinkres);

            return wrap_procedure_results_in_successful_accepted_reply(readlinkres_size, readlinkres_buffer, "nfs/ReadLinkRes");
        }
        else{
            perror_msg("serve_nfs_procedure_5_read_from_symbolic_link: failed reading the path inside the symbolic link at absolute path '%s'\n", symlink_absolute_path);

            nfs__fhandle__free_unpacked(symlink_fhandle, NULL);

            return create_system_error_accepted_reply();
        }
    }
    // null-terminate the pathname
    target_path[bytes_read] = '\0';

    // build the procedure results
    Nfs__ReadLinkRes readlinkres = NFS__READ_LINK_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;
    readlinkres.nfs_status = &nfs_status;
    readlinkres.body_case = NFS__READ_LINK_RES__BODY_DATA;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = target_path;

    readlinkres.data = &path;

    // serialize the procedure results
    size_t readlinkres_size = nfs__read_link_res__get_packed_size(&readlinkres);
    uint8_t *readlinkres_buffer = malloc(readlinkres_size);
    nfs__read_link_res__pack(&readlinkres, readlinkres_buffer);

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(readlinkres_size, readlinkres_buffer, "nfs/ReadLinkRes");

    nfs__fhandle__free_unpacked(symlink_fhandle, NULL);

    return accepted_reply;
}