#include "nfsproc.h"

/*
* Runs the NFSPROC_LOOKUP procedure (4).
*
* Takes a RPC credential+verifier pair corresponding to a supported authentication flavor. The provided
* credential and verifier must be structurally validated (i.e. no NULL fields and correspond to a supported authentication
* flavor) before being passed here.
* This procedure must not be given AUTH_NONE credential+verifier pair.
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_4_look_up_file_name(Rpc__OpaqueAuth *credential, Rpc__OpaqueAuth *verifier, Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/DirOpArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: expected nfs/DirOpArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__DirOpArgs *diropargs = nfs__dir_op_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(diropargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed to unpack DirOpArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(diropargs->dir == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'dir' in DirOpArgs is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FHandle *directory_fhandle = diropargs->dir;
    if(directory_fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: FHandle->nfs_filehandle is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    if(diropargs->name == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'name' in DirOpArgs is null\n");

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        return create_garbage_args_accepted_reply();
    }
    Nfs__FileName *file_name = diropargs->name;
    if(file_name->filename == NULL) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'filename' in DirOpArgs is null\n");

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
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }

    // get the attributes of this directory before the lookup, to check that it is actually a directory
    Nfs__FAttr directory_fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(directory_absolute_path, &directory_fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed getting file attributes for file at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this directory back to its absolute path
        return create_system_error_accepted_reply();
    }
    // only directories can be looked up using LOOKUP
    if(directory_fattr.nfs_ftype->ftype != NFS__FTYPE__NFDIR) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: 'lookup' procedure called on a non-directory '%s'\n", directory_absolute_path);

        // build the procedure results
        Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_NOTDIR);

        // serialize the procedure results
        size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
        uint8_t *diropres_buffer = malloc(diropres_size);
        nfs__dir_op_res__pack(diropres, diropres_buffer);

        clean_up_fattr(&directory_fattr);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(diropres->nfs_status);
        free(diropres->default_case);
        free(diropres);

        return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
    }
    clean_up_fattr(&directory_fattr);

    // check that the file client wants to lookup exists
    char *file_absolute_path = get_file_absolute_path(directory_absolute_path, file_name->filename);
    struct stat file_stat;
    error_code = lstat(file_absolute_path, &file_stat);
    if(error_code < 0) {
        if(errno == ENOENT) {
            Nfs__Stat nfs_stat;
            switch(errno) {
                case ENOENT:
                    nfs_stat = NFS__STAT__NFSERR_NOENT;
                    fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: attempted 'lookup' on a file at absolute path '%s' which does not exist\n", file_absolute_path);
                    break;
            }

            // build the procedure results
            Nfs__DirOpRes *diropres = create_default_case_dir_op_res(nfs_stat);

            // serialize the procedure results
            size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
            uint8_t *diropres_buffer = malloc(diropres_size);
            nfs__dir_op_res__pack(diropres, diropres_buffer);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);
            free(diropres->nfs_status);
            free(diropres->default_case);
            free(diropres);

            return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
        }
        else{
            perror_msg("serve_nfs_procedure_4_look_up_file_name: failed checking if file to be created at absolute path '%s' already exists", file_absolute_path);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);

            return create_system_error_accepted_reply();
        }
    }

    // check permissions
    if(credential->flavor == RPC__AUTH_FLAVOR__AUTH_SYS) {
        int stat = check_lookup_proc_permissions(directory_absolute_path, credential->auth_sys->uid, credential->auth_sys->gid);
        if(stat < 0) {
            fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed checking LOOKUP permissions for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, stat);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);

            return create_system_error_accepted_reply();
        }

        // client does not have correct permission to set attributes of this file/directory
        if(stat == 1) {
            // build the procedure results
            Nfs__DirOpRes *diropres = create_default_case_dir_op_res(NFS__STAT__NFSERR_ACCES);

            // serialize the procedure results
            size_t diropres_size = nfs__dir_op_res__get_packed_size(diropres);
            uint8_t *diropres_buffer = malloc(diropres_size);
            nfs__dir_op_res__pack(diropres, diropres_buffer);

            free(file_absolute_path);
            nfs__dir_op_args__free_unpacked(diropargs, NULL);
            free(diropres->nfs_status);
            free(diropres->default_case);
            free(diropres);

            return wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");
        }
    }
    // there's no other supported authentication flavor yet (this function only receives credential+verifier pairs with supported authentication flavor)

    // create a NFS filehandle for the looked up file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    error_code = create_nfs_filehandle(file_absolute_path, &file_nfs_filehandle, &inode_cache);
    if(error_code > 0) {
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed creating a NFS filehandle for file at absolute path '%s' with error code %d\n", directory_absolute_path, error_code);

        free(file_absolute_path);
        nfs__dir_op_args__free_unpacked(diropargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've checked that the looked up file exists
        return create_system_error_accepted_reply();
    }

    // get the attributes of the looked up file
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_4_look_up_file_name: failed getting attributes for file/directory at absolute path '%s' with error code %d \n", file_absolute_path, error_code);

        nfs__dir_op_args__free_unpacked(diropargs, NULL);
        free(file_absolute_path);
        // remove the inode cache mapping for this file/directory that we added when creating the NFS filehandle, as LOOKUP was unsuccessful
        remove_inode_mapping_by_inode_number(file_nfs_filehandle.inode_number, &inode_cache);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've created a NFS filehandle for this file (we successfully read stat.st_ino)
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__DirOpRes diropres = NFS__DIR_OP_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;
    diropres.nfs_status = &nfs_status;
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

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(diropres_size, diropres_buffer, "nfs/DirOpRes");

    nfs__dir_op_args__free_unpacked(diropargs, NULL);

    clean_up_fattr(&fattr);

    free(file_absolute_path);

    return accepted_reply;
}