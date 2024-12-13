#include "nfsproc.h"

/*
* Runs the NFSPROC_GETATTR procedure (1).
*/
Rpc__AcceptedReply serve_nfs_procedure_1_get_file_attributes(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: expected nfs/FHandle but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__FHandle *fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(fhandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: failed to unpack FHandle\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: FHandle->nfs_filehandle is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *nfs_filehandle = fhandle->nfs_filehandle;
    ino_t inode_number = nfs_filehandle->inode_number;

    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode the inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such file or directory
        fprintf(stdout, "serve_nfs_procedure_1_get_file_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        // build the procedure results
        Nfs__AttrStat *attr_stat = create_default_case_attr_stat(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t attr_stat_size = nfs__attr_stat__get_packed_size(attr_stat);
        uint8_t *attr_stat_buffer = malloc(attr_stat_size);
        nfs__attr_stat__pack(attr_stat, attr_stat_buffer);

        nfs__fhandle__free_unpacked(fhandle, NULL);
        free(attr_stat->nfs_status);
        free(attr_stat->default_case);
        free(attr_stat);

        return wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");
    }

    Nfs__FAttr fattr = NFS__FATTR__INIT;
    int error_code = get_attributes(file_absolute_path, &fattr);
    if(error_code > 0) {
        fprintf(stderr, "serve_nfs_procedure_1_get_file_attributes: failed getting attributes for file/directory at absolute path '%s' with error code %d\n", file_absolute_path, error_code);

        nfs__fhandle__free_unpacked(fhandle, NULL);

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

    Rpc__AcceptedReply accepted_reply = wrap_procedure_results_in_successful_accepted_reply(attr_stat_size, attr_stat_buffer, "nfs/AttrStat");

    nfs__fhandle__free_unpacked(fhandle, NULL);

    clean_up_fattr(&fattr);

    return accepted_reply;
}