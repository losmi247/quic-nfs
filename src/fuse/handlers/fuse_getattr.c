#include "handlers.h"

typedef struct GetattrData {
    char *path;
    struct stat *stbuf;
} GetattrData;

void *blocking_getattr(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    GetattrData *getattr_data = (GetattrData *) callback_data->return_data;

    struct stat *stbuf = getattr_data->stbuf;

    memset(stbuf, 0, sizeof(struct stat));

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, getattr_data->path, &file_type);
    if(file_fhandle == NULL) {
        printf("nfs_getattr: failed to resolve the path %s to a file\n", getattr_data->path);

        callback_data->error_code = -ENOENT;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(rpc_connection_context, *file_fhandle, attrstat);
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

    stbuf->st_dev = attrstat->attributes->fsid;
    stbuf->st_ino = attrstat->attributes->fileid;
    stbuf->st_mode = attrstat->attributes->mode;
    stbuf->st_nlink = attrstat->attributes->nlink;

    stbuf->st_uid = attrstat->attributes->uid;
    stbuf->st_gid = attrstat->attributes->gid;

    stbuf->st_rdev = attrstat->attributes->rdev;
    stbuf->st_size = attrstat->attributes->size;
    stbuf->st_blksize = attrstat->attributes->blocksize;
    stbuf->st_blocks = attrstat->attributes->blocks;

    stbuf->st_atim.tv_sec = attrstat->attributes->atime->seconds;
    stbuf->st_atim.tv_nsec = attrstat->attributes->atime->useconds;

    stbuf->st_mtim.tv_sec = attrstat->attributes->mtime->seconds;
    stbuf->st_mtim.tv_nsec = attrstat->attributes->mtime->useconds;

    stbuf->st_ctim.tv_sec = attrstat->attributes->ctime->seconds;
    stbuf->st_ctim.tv_nsec = attrstat->attributes->ctime->useconds;
    
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
* Handles the FUSE call to get file attributes.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    /*memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    return -ENOENT;*/

    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    GetattrData getattr_data;
    getattr_data.path = discard_const(path);
	getattr_data.stbuf = stbuf;

    callback_data.return_data = &getattr_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_getattr, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

	wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    return callback_data.error_code;
}
 