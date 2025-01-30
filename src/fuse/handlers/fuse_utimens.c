#include "handlers.h"

#include <time.h>

typedef struct UtimensData {
    char *path;
    
    time_t atime_sec;
    long atime_nsec;
    time_t mtime_sec;
    long mtime_nsec;
} UtimensData;

void *blocking_utimens(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    UtimensData *utimens_data = (UtimensData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, utimens_data->path, &file_type, &error_code);
    if(file_fhandle == NULL) {
        printf("nfs_getattr: failed to resolve the path %s to a file\n", utimens_data->path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = file_fhandle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = -1;
    sattr.uid = -1;
    sattr.gid = -1;
    sattr.size = -1;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;

    if(utimens_data->atime_sec == UTIME_OMIT || utimens_data->atime_nsec == UTIME_OMIT) {
        atime.seconds = atime.useconds = -1;
    }
    else if(utimens_data->atime_nsec == UTIME_NOW) {
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);

        atime.seconds = current_time.tv_sec;
        atime.useconds = current_time.tv_nsec / 1000;
    }
    else {
        atime.seconds = utimens_data->atime_sec;
        atime.useconds = utimens_data->atime_nsec / 1000;
    }

    if(utimens_data->mtime_sec == UTIME_OMIT || utimens_data->mtime_nsec == UTIME_OMIT) {
        mtime.seconds = mtime.useconds = -1;
    }
    else if(utimens_data->mtime_nsec == UTIME_NOW) {
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);

        mtime.seconds = current_time.tv_sec;
        mtime.useconds = current_time.tv_nsec / 1000;
    }
    else {
        mtime.seconds = utimens_data->mtime_sec;
        mtime.useconds = utimens_data->mtime_nsec / 1000;
    }
    
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
        printf("Error: Failed to set attributes of the file with status %s\n", string_status);
        free(string_status);

        nfs__attr_stat__free_unpacked(attrstat, NULL);

        callback_data->error_code = map_nfs_error(attrstat->nfs_status->stat);
        
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
* Handles the FUSE call to update times of the given file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi)  {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    UtimensData utimens_data;
    utimens_data.path = discard_const(path);
	utimens_data.atime_sec = tv[0].tv_sec;
    utimens_data.atime_nsec = tv[0].tv_nsec;
    utimens_data.mtime_sec = tv[1].tv_sec;
    utimens_data.mtime_nsec = tv[1].tv_nsec;

    callback_data.return_data = &utimens_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_utimens, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

	wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    return callback_data.error_code;
}
 