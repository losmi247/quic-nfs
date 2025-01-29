#include "handlers.h"

typedef struct UnlinkData {
    char *containing_directory_path;
    char *file_name;
} UnlinkData;

void *blocking_unlink(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;
    UnlinkData *unlink_data = (UnlinkData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *containing_directory_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, unlink_data->containing_directory_path, &file_type, &error_code);
    if(containing_directory_fhandle == NULL) {
        printf("nfs_unlink: failed to resolve the path %s to a file\n", unlink_data->containing_directory_path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = containing_directory_fhandle;

    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = unlink_data->file_name;
    diropargs.name = &filename;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_10_remove_file(rpc_connection_context, diropargs, nfsstat);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(nfsstat);

        callback_data->error_code = -EIO;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    pthread_mutex_unlock(&nfs_mutex);

    if(validate_nfs_nfs_stat(nfsstat) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = -EIO;
        
        goto signal;
    }

    if(nfsstat->stat == NFS__STAT__NFSERR_ACCES) {
        printf("Error: Permission denied\n");
        
        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = -EACCES;
        
        goto signal;
    }
    else if(nfsstat->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(nfsstat->stat);
        printf("Error: Failed to remove directory with status %s\n", string_status);
        free(string_status);

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = map_nfs_error(nfsstat->stat);
        
        goto signal;
    }

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
    free(containing_directory_fhandle->nfs_filehandle);
    free(containing_directory_fhandle);

    callback_data->error_code = 0;

signal:
    pthread_mutex_lock(&callback_data->lock);
    callback_data->is_finished = 1;
    pthread_cond_signal(&callback_data->cond);
    pthread_mutex_unlock(&callback_data->lock);

    return NULL;
}

/*
* Handles the FUSE call to remove a file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_unlink(const char *path) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    char *dir_copy = strdup(path);
    char *path_copy = strdup(path);

    UnlinkData unlink_data;
    unlink_data.containing_directory_path = dirname(dir_copy);
    unlink_data.file_name = basename(path_copy);

    callback_data.return_data = &unlink_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_unlink, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

    wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    free(dir_copy);
    free(path_copy);

    return callback_data.error_code;
}
