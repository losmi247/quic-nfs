#include "nfsproc.h"

/*
* Runs the NFSPROC_WRITE procedure (8).
*/
Rpc__AcceptedReply serve_nfs_procedure_8_write_to_file(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/WriteArgs") != 0) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: expected nfs/WriteArgs but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__WriteArgs *writeargs = nfs__write_args__unpack(NULL, parameters->value.len, parameters->value.data);
    if(writeargs == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed to unpack WriteArgs\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(writeargs->file == NULL) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: 'file' in WriteArgs is null\n");

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
        free(attr_stat->nfs_status);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    // get the attributes of the looked up file before the write, to check that the file is not a directory
    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }
    // all file types except for directories can be written to as files
    if(fattr.nfs_ftype->ftype == NFS__FTYPE__NFDIR) {
        // if the file is actually directory, return AttrStat with 'directory specified in a non-directory operation' status
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: a directory '%s' was specified for 'write' which is a non-directory operation\n", file_absolute_path);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_ISDIR);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        clean_up_fattr(&fattr);
        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->nfs_status);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }
    clean_up_fattr(&fattr);

    // check if client requested to write too much data in a single RPC
    if(writeargs->nfsdata.len > NFS_MAXDATA) {
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: attempted 'write' of %ld bytes to file at absolute path '%s', but max write allowed in a single RPC is %d bytes\n", writeargs->nfsdata.len, file_absolute_path, NFS_MAXDATA);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_FBIG); // FBIG error is not intended for this, but it's the most similar in meaning

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->nfs_status);
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
                fprintf(stderr, "serve_nfs_procedure_8_write_to_file: attempted write that would exceed file size limits, to file at absolute path '%s'\n", file_absolute_path);
            case 5:
                nfs_stat = NFS__STAT__NFSERR_IO;
                fprintf(stderr, "serve_nfs_procedure_8_write_to_file: physical IO error occurred while trying to write to file at absolute path '%s'\n", file_absolute_path);
            case 6:
                nfs_stat = NFS__STAT__NFSERR_NOSPC;
                fprintf(stderr, "serve_nfs_procedure_8_write_to_file: no space left on device to write to file at absolute path '%s'\n", file_absolute_path);
        }

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(nfs_stat);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__write_args__free_unpacked(writeargs, NULL);
        free(attr_stat->nfs_status);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }
    else if(error_code > 0) {
        // we failed writing to this file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed writing to file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        return create_system_error_accepted_reply();
    }

    // get the attributes of the file after the write - this is what we return to client
    Nfs__FAttr fattr_after_write = NFS__FATTR__INIT;
    error_code = get_attributes(file_absolute_path, &fattr_after_write);
    if(error_code > 0) {
        // we failed getting attributes for this file
        fprintf(stderr, "serve_nfs_procedure_8_write_to_file: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__write_args__free_unpacked(writeargs, NULL);

        // return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded the NFS filehandle for this file back to its absolute path
        return create_system_error_accepted_reply();
    }

    // build the procedure results
    Nfs__AttrStat attr_stat = NFS__ATTR_STAT__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;

    attr_stat.nfs_status = &nfs_status;
    attr_stat.body_case = NFS__ATTR_STAT__BODY_ATTRIBUTES;
    attr_stat.attributes = &fattr_after_write;

    // serialize the procedure results
    size_t attr_stat_size = nfs__attr_stat__get_packed_size(&attr_stat);
    uint8_t *attr_stat_buffer = malloc(attr_stat_size);
    nfs__attr_stat__pack(&attr_stat, attr_stat_buffer);

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

    nfs__write_args__free_unpacked(writeargs, NULL);

    clean_up_fattr(&fattr_after_write);

    return accepted_reply;
}