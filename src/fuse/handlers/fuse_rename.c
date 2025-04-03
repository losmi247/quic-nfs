#include "handlers.h"

typedef struct RenameData {
    char *old_containing_directory_path;
    char *old_file_name;

    char *new_containing_directory_path;
    char *new_file_name;
} RenameData;

void *blocking_rename(void *arg) {
    CallbackData *callback_data = (CallbackData *)arg;
    RenameData *rename_data = (RenameData *)callback_data->return_data;

    Nfs__FType src_dir_file_type;
    int error_code;
    Nfs__FHandle *old_containing_directory_fhandle =
        resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle,
                              rename_data->old_containing_directory_path, &src_dir_file_type, &error_code);
    if (old_containing_directory_fhandle == NULL) {
        printf("nfs_rename: failed to resolve the path %s to a file\n", rename_data->old_containing_directory_path);

        callback_data->error_code = -error_code;

        goto signal;
    }

    Nfs__FType dest_dir_file_type;
    Nfs__FHandle *new_containing_directory_fhandle =
        resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle,
                              rename_data->new_containing_directory_path, &dest_dir_file_type, &error_code);
    if (new_containing_directory_fhandle == NULL) {
        printf("nfs_rename: failed to resolve the path %s to a file\n", rename_data->new_containing_directory_path);

        callback_data->error_code = -error_code;

        goto signal;
    }

    Nfs__RenameArgs renameargs = NFS__RENAME_ARGS__INIT;

    Nfs__DirOpArgs from_diropargs = NFS__DIR_OP_ARGS__INIT;
    from_diropargs.dir = old_containing_directory_fhandle;
    Nfs__FileName old_filename = NFS__FILE_NAME__INIT;
    old_filename.filename = rename_data->old_file_name;
    from_diropargs.name = &old_filename;

    renameargs.from = &from_diropargs;

    Nfs__DirOpArgs to_diropargs = NFS__DIR_OP_ARGS__INIT;
    to_diropargs.dir = new_containing_directory_fhandle;
    Nfs__FileName new_filename = NFS__FILE_NAME__INIT;
    new_filename.filename = rename_data->new_file_name;
    to_diropargs.name = &new_filename;

    renameargs.to = &to_diropargs;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_11_rename_file(rpc_connection_context, renameargs, nfsstat);
    if (status != 0) {
        printf("Error: Invalid RPC reply received from the server with status %d\n", status);

        free(nfsstat);

        callback_data->error_code = -EIO;

        goto signal;
    }

    if (validate_nfs_nfs_stat(nfsstat) > 0) {
        printf("Error: Invalid NFS procedure result received from the server\n");

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = -EIO;

        goto signal;
    }

    if (nfsstat->stat == NFS__STAT__NFSERR_ACCES) {
        printf("Error: Permission denied\n");

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = -EACCES;

        goto signal;
    } else if (nfsstat->stat != NFS__STAT__NFS_OK) {
        char *string_status = nfs_stat_to_string(nfsstat->stat);
        printf("Error: Failed to rename a file with status %s\n", string_status);
        free(string_status);

        nfs__nfs_stat__free_unpacked(nfsstat, NULL);

        callback_data->error_code = map_nfs_error(nfsstat->stat);

        goto signal;
    }

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free(old_containing_directory_fhandle->nfs_filehandle);
    free(old_containing_directory_fhandle);

    free(new_containing_directory_fhandle->nfs_filehandle);
    free(new_containing_directory_fhandle);

    callback_data->error_code = 0;

signal:
    pthread_mutex_lock(&callback_data->lock);
    callback_data->is_finished = 1;
    pthread_cond_signal(&callback_data->cond);
    pthread_mutex_unlock(&callback_data->lock);

    return NULL;
}

/*
 * Handles the FUSE call to rename a file.
 *
 * Returns 0 on success and the appropriate negative error code on failure.
 */
int nfs_rename(const char *from, const char *to, unsigned int flags) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    // make copies of the paths since dirname() and basename() modify arguments
    char *from_dir_copy = strdup(from);
    char *from_path_copy = strdup(from);
    char *to_dir_copy = strdup(to);
    char *to_path_copy = strdup(to);

    RenameData rename_data;
    char *old_directory_path =
        dirname(from_dir_copy); // returns "." if there are no '/'s in the path (i.e. just file name 'file')
    rename_data.old_containing_directory_path = strcmp(old_directory_path, ".") == 0 ? "/" : old_directory_path;
    rename_data.old_file_name = basename(from_path_copy);

    char *new_directory_path =
        dirname(to_dir_copy); // returns "." if there are no '/'s in the path (i.e. just file name 'file')
    rename_data.new_containing_directory_path = strcmp(new_directory_path, ".") == 0 ? "/" : new_directory_path;
    rename_data.new_file_name = basename(to_path_copy);

    callback_data.return_data = &rename_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if (pthread_create(&blocking_thread, NULL, blocking_rename, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

    wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    free(from_dir_copy);
    free(from_path_copy);
    free(to_dir_copy);
    free(to_path_copy);

    return callback_data.error_code;
}
