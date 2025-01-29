#include "handlers.h"

typedef struct TruncateData {
    char *path;
    off_t new_file_size;
} TruncateData;

void *blocking_truncate(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    TruncateData *truncate_data = (TruncateData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, truncate_data->path, &file_type);
    if(file_fhandle == NULL) {
        printf("nfs_truncate: failed to resolve the path %s to a file\n", truncate_data->path);

        callback_data->error_code = -ENOENT;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = file_fhandle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = -1;
    sattr.uid = -1;
    sattr.gid = -1;
    sattr.size = truncate_data->new_file_size;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = -1;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(rpc_connection_context, sattrargs, attrstat);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(attrstat);

        callback_data->error_code = -EIO;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    pthread_mutex_unlock(&nfs_mutex);

    if(validate_nfs_attr_stat(attrstat) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__attr_stat__free_unpacked(attrstat, NULL);

        callback_data->error_code = -EIO;
        
        goto signal;
    }

    if(attrstat->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("Error: Permission denied\n");
        
        nfs__attr_stat__free_unpacked(attrstat, NULL);

        callback_data->error_code = -EACCES;
        
        goto signal;
    }
    else if(attrstat->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(attrstat->nfs_status->stat);
        printf("Error: Failed to get attributes of the file with status %s\n", string_status);
        free(string_status);

        nfs__attr_stat__free_unpacked(attrstat, NULL);

        callback_data->error_code = -(attrstat->nfs_status->stat);
        
        goto signal;
    }
    
    nfs__attr_stat__free_unpacked(attrstat, NULL);
    free(file_fhandle->nfs_filehandle);
    free(file_fhandle);

    callback_data->error_code = 0;

signal:
    pthread_mutex_lock(&callback_data->lock);
    callback_data->is_finished = 1;
    pthread_cond_signal(&callback_data->cond);
    pthread_mutex_unlock(&callback_data->lock);

    return NULL;
}

/*
* Handles the FUSE call to truncate a file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    TruncateData truncate_data;
    truncate_data.path = discard_const(path);
	truncate_data.new_file_size = size;

    callback_data.return_data = &truncate_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_truncate, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

	wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    return callback_data.error_code;
}
 