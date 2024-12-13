#include "nfsproc.h"

/*
* Runs the NFSPROC_READ procedure (6).
*/
Rpc__AcceptedReply serve_nfs_procedure_6_read_from_file(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/ReadArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: expected nfs/ReadArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__ReadArgs *readargs = nfs__read_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(readargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed to unpack ReadArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(readargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: 'file' in ReadArgs is null\n");

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
        free(readres->nfs_status);
        free(readres->default_case);
        free(readres);

        return wrap_procedure_results_in_successful_accepted_reply(readres_size, readres_buffer, "nfs/ReadRes");
    }

    // get the attributes of the looked up file before the read, to check that the file is not a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // all file types except for directories can be read as files
    if(fattr.nfs_ftype->ftype == NFS__FTYPE__NFDIR) {
        // if the file is actually directory, return ReadRes with 'directory specified in a non-directory operation' status
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: a directory '%s' was specified for 'read' which is a non-directory operation\n", file_absolute_path);

        // build the procedure results
        Nfs__ReadRes *readres = create_default_case_read_res(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t readres_size = nfs__read_res__get_packed_size(readres);
        uint8_t *readres_buffer = malloc(readres_size);
        nfs__read_res__pack(readres, readres_buffer);

        clean_up_fattr(&fattr);
        nfs__read_args__free_unpacked(readargs, NULL);
        free(readres->default_case);
        free(readres);

        return wrap_procedure_results_in_successful_accepted_reply(readres_size, readres_buffer, "nfs/ReadRes");
    }
    clean_up_fattr(&fattr);

    // if client requested to read too much data in a single RPC, truncate the read down to NFS_MAXDATA bytes
    if(readargs->count > NFS_MAXDATA) {
        readargs->count = NFS_MAXDATA;
    }

    // read from the file
    uint8_t *read_data = malloc(sizeof(uint8_t) * readargs->count);
    size_t bytes_read;
    error_code = read_from_file(file_absolute_path, readargs->offset, readargs->count, read_data, &bytes_read);
    if(error_code > 0) {
        // we failed to read from this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed to read from file at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }

    // get the attributes of the looked up file after the read - this is what we return to client
    Nfs__FAttr fattr_after_read = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr_after_read);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_6_read_from_file: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__read_args__free_unpacked(readargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    
    // build the procedure results
    Nfs__ReadRes readres = NFS__READ_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;
    readres.nfs_status = &nfs_status;
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

    clean_up_fattr(&fattr_after_read);

    return accepted_reply;
}