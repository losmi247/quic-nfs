#include "handlers.h"

typedef struct MknodData {
    char *containing_directory_path;
    char *new_file_name;

    mode_t mode;
    dev_t rdev;
} MknodData;

void *blocking_mknod(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    MknodData *mknod_data = (MknodData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *containing_directory_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, mknod_data->containing_directory_path, &file_type, &error_code);
    if(containing_directory_fhandle == NULL) {
        printf("nfs_mknod: failed to resolve the path %s to a file\n", mknod_data->containing_directory_path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = containing_directory_fhandle;

    Nfs__FileName filename = NFS__FILE_NAME__INIT;
    filename.filename = mknod_data->new_file_name;
    diropargs.name = &filename;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mknod_data->mode;
    sattr.uid = rpc_connection_context->credential->auth_sys->uid;         // the REPL that creates the file is the owner of the file
    sattr.gid = rpc_connection_context->credential->auth_sys->gid;
    sattr.size = -1;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = -1;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    createargs.attributes = &sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_9_create_file(rpc_connection_context, createargs, diropres);
    if(status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(diropres);

        callback_data->error_code = -EIO;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    pthread_mutex_unlock(&nfs_mutex);

    if(validate_nfs_dir_op_res(diropres) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        callback_data->error_code = -EIO;
        
        goto signal;
    }

    if(diropres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
        printf("Error: Permission denied\n");
        
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        callback_data->error_code = -EACCES;
        
        goto signal;
    }
    else if(diropres->nfs_status->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(diropres->nfs_status->stat);
        printf("Error: Failed to create file with status %s\n", string_status);
        free(string_status);

        nfs__dir_op_res__free_unpacked(diropres, NULL);

        callback_data->error_code = map_nfs_error(diropres->nfs_status->stat);
        
        goto signal;
    }
    
    nfs__dir_op_res__free_unpacked(diropres, NULL);
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
* Handles the FUSE call to create a file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    // make copies of the path because dirname() and basename() modify their arguments
    char *dir_copy = strdup(path);
    char *path_copy = strdup(path);

    MknodData mknod_data;
    char *directory_path = dirname(dir_copy);   // returns "." if there are no '/'s in the path (i.e. just file name 'file')
    mknod_data.containing_directory_path = strcmp(directory_path,".") == 0 ? "/" : directory_path;
    mknod_data.new_file_name = basename(path_copy);

    mknod_data.mode = mode;
    mknod_data.rdev = rdev;

    callback_data.return_data = &mknod_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_mknod, &callback_data) != 0) {
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
 