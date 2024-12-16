#include "nfsproc.h"

#include "sys/statvfs.h"

/*
* Runs the NFSPROC_STATFS procedure (17).
*
* The user of this function takes the responsibility to deallocate the received AcceptedReply
* using the 'free_accepted_reply()' function.
*/
Rpc__AcceptedReply *serve_nfs_procedure_17_get_filesystem_attributes(Google__Protobuf__Any *parameters) {
    // check parameters are of expected type for this procedure
    if(parameters->type_url == NULL || strcmp(parameters->type_url, "nfs/FHandle") != 0) {
        fprintf(stderr, "serve_nfs_procedure_17_get_filesystem_attributes: expected nfs/FHandle but received %s\n", parameters->type_url);
        
        return create_garbage_args_accepted_reply();
    }

    // deserialize parameters
    Nfs__FHandle *fhandle = nfs__fhandle__unpack(NULL, parameters->value.len, parameters->value.data);
    if(fhandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_17_get_filesystem_attributes: failed to unpack FHandle\n");
        
        return create_garbage_args_accepted_reply();
    }
    if(fhandle->nfs_filehandle == NULL) {
        fprintf(stderr, "serve_nfs_procedure_17_get_filesystem_attributes: FHandle->nfs_filehandle is null\n");

        nfs__fhandle__free_unpacked(fhandle, NULL);

        return create_garbage_args_accepted_reply();
    }

    NfsFh__NfsFileHandle *nfs_filehandle = fhandle->nfs_filehandle;
    ino_t inode_number = nfs_filehandle->inode_number;

    char *file_absolute_path = get_absolute_path_from_inode_number(inode_number, inode_cache);
    if(file_absolute_path == NULL) {
        // we couldn't decode the inode number back to a file/directory - we assume the client gave us a wrong NFS filehandle, i.e. no such file or directory
        fprintf(stdout, "serve_nfs_procedure_17_get_filesystem_attributes: failed to decode inode number %ld back to a file/directory\n", inode_number);

        // build the procedure results
        Nfs__StatFsRes *statfsres = create_default_case_stat_fs_res(NFS__STAT__NFSERR_NOENT);

        // serialize the procedure results
        size_t statfsres_size = nfs__stat_fs_res__get_packed_size(statfsres);
        uint8_t *statfsres_buffer = malloc(statfsres_size);
        nfs__stat_fs_res__pack(statfsres, statfsres_buffer);
    
        nfs__fhandle__free_unpacked(fhandle, NULL);
        free(statfsres->nfs_status);
        free(statfsres->default_case);
        free(statfsres);

        return wrap_procedure_results_in_successful_accepted_reply(statfsres_size, statfsres_buffer, "nfs/StatFsRes");
    }

    // get file system attributes for the filesystem this file is on
    struct statvfs fs_stat;
    int error_code = statvfs(file_absolute_path, &fs_stat);
    if(error_code < 0) {
        if(errno == EIO) {
            // build the procedure results
            Nfs__StatFsRes *statfsres = create_default_case_stat_fs_res(NFS__STAT__NFSERR_IO);

            // serialize the procedure results
            size_t statfsres_size = nfs__stat_fs_res__get_packed_size(statfsres);
            uint8_t *statfsres_buffer = malloc(statfsres_size);
            nfs__stat_fs_res__pack(statfsres, statfsres_buffer);
        
            nfs__fhandle__free_unpacked(fhandle, NULL);
            free(statfsres->nfs_status);
            free(statfsres->default_case);
            free(statfsres);

            return wrap_procedure_results_in_successful_accepted_reply(statfsres_size, statfsres_buffer, "nfs/StatFsRes");
        }
        else {
            perror_msg("serve_nfs_procedure_17_get_filesystem_attributes: failed getting attributes of the filesystem\n");

            nfs__fhandle__free_unpacked(fhandle, NULL);

            // we return AcceptedReply with SYSTEM_ERR, as this shouldn't happen once we've decoded inode number to a file
            return create_system_error_accepted_reply();
        }
    }

    // build the procedure results
    Nfs__StatFsRes statfsres = NFS__STAT_FS_RES__INIT;

    Nfs__NfsStat nfs_status = NFS__NFS_STAT__INIT;
    nfs_status.stat = NFS__STAT__NFS_OK;

    statfsres.nfs_status = &nfs_status;
    statfsres.body_case = NFS__STAT_FS_RES__BODY_FS_INFO;

    Nfs__FsInfo fsinfo = NFS__FS_INFO__INIT;
    fsinfo.tsize = fs_stat.f_bsize;
    fsinfo.bsize = fs_stat.f_frsize;
    fsinfo.blocks = fs_stat.f_blocks;
    fsinfo.bfree = fs_stat.f_bfree;
    fsinfo.bavail = fs_stat.f_bavail;

    statfsres.fs_info = &fsinfo;

    // serialize the procedure results
    size_t statfsres_size = nfs__stat_fs_res__get_packed_size(&statfsres);
    uint8_t *statfsres_buffer = malloc(statfsres_size);
    nfs__stat_fs_res__pack(&statfsres, statfsres_buffer);

    Rpc__AcceptedReply *accepted_reply = wrap_procedure_results_in_successful_accepted_reply(statfsres_size, statfsres_buffer, "nfs/StatFsRes");

    nfs__fhandle__free_unpacked(fhandle, NULL);

    return accepted_reply;
}