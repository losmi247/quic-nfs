#include "handlers.h"

typedef struct ReadlinkData {
    char *path;

    char *buffer;
    size_t max_bytes_to_read;
} ReadlinkData;

void *blocking_readlink(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;
    ReadlinkData *readlink_data = (ReadlinkData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, readlink_data->path, &file_type, &error_code);
    if(file_fhandle == NULL) {
        printf("nfs_readlink: failed to resolve the path %s to a file\n", readlink_data->path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__ReadLinkRes *readlinkres = malloc(sizeof(Nfs__ReadLinkRes));
    int status = nfs_procedure_5_read_from_symbolic_link(rpc_connection_context, *file_fhandle, readlinkres);
    if (status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(readlinkres);

        callback_data->error_code = -EIO;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    pthread_mutex_unlock(&nfs_mutex);

    if(validate_nfs_read_link_res(readlinkres) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__read_link_res__free_unpacked(readlinkres, NULL);

        callback_data->error_code = -EIO;
        
        goto signal;
    }

    if(readlinkres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("Error: Permission denied\n");
        
        nfs__read_link_res__free_unpacked(readlinkres, NULL);

        callback_data->error_code = -EACCES;
        
        goto signal;
    }
    else if(readlinkres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(readlinkres->nfs_status->stat);
        printf("Error: Failed to read from symbolic link with status %s\n", string_status);
        free(string_status);

        nfs__read_link_res__free_unpacked(readlinkres, NULL);

        callback_data->error_code = map_nfs_error(readlinkres->nfs_status->stat);
        
        goto signal;
    }

    // copy the symlink target path to the output buffer
    strncpy(readlink_data->buffer, readlinkres->data->path, readlink_data->max_bytes_to_read);
    readlink_data->buffer[readlink_data->max_bytes_to_read - 1] = '\0';     // null terminate the retrieved path

    free(file_fhandle->nfs_filehandle);
    free(file_fhandle);
    nfs__read_link_res__free_unpacked(readlinkres, NULL);

    callback_data->error_code = 0;

signal:
    pthread_mutex_lock(&callback_data->lock);
    callback_data->is_finished = 1;
    pthread_cond_signal(&callback_data->cond);
    pthread_mutex_unlock(&callback_data->lock);

    return NULL;
}

/*
* Handles the FUSE call to read from a symbolic link.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_readlink(const char *path, char *buffer, size_t size) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    ReadlinkData readlink_data;
    readlink_data.path = discard_const(path);
    readlink_data.buffer = buffer;
    readlink_data.max_bytes_to_read = size;

    callback_data.return_data = &readlink_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if (pthread_create(&blocking_thread, NULL, blocking_readlink, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

    wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    return callback_data.error_code;
}
