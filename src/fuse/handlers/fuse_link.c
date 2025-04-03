#include "handlers.h"

typedef struct LinkData {
    char *target_path;

    char *containing_directory_path;
    char *link_name;
} LinkData;

void *blocking_link(void *arg) {
    CallbackData *callback_data = (CallbackData *)arg;
    LinkData *link_data = (LinkData *)callback_data->return_data;

    Nfs__FType containing_directory_type;
    int error_code;
    Nfs__FHandle *containing_directory_fhandle =
        resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, link_data->containing_directory_path,
                              &containing_directory_type, &error_code);
    if (containing_directory_fhandle == NULL) {
        printf("nfs_link: failed to resolve the path %s to a file\n", link_data->containing_directory_path);

        callback_data->error_code = -error_code;

        goto signal;
    }

    Nfs__FType target_file_ftype;
    Nfs__FHandle *target_file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle,
                                                              link_data->target_path, &target_file_ftype, &error_code);
    if (target_file_fhandle == NULL) {
        printf("nfs_link: failed to resolve the path %s to a file\n", link_data->target_path);

        callback_data->error_code = -error_code;

        goto signal;
    }

    Nfs__LinkArgs linkargs = NFS__LINK_ARGS__INIT;

    linkargs.from = target_file_fhandle;

    Nfs__DirOpArgs to_diropargs = NFS__DIR_OP_ARGS__INIT;
    to_diropargs.dir = containing_directory_fhandle;
    Nfs__FileName to_filename = NFS__FILE_NAME__INIT;
    to_filename.filename = link_data->link_name;
    to_diropargs.name = &to_filename;

    linkargs.to = &to_diropargs;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_12_create_link_to_file(rpc_connection_context, linkargs, nfsstat);
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
        printf("Error: Failed to create symbolic link with status %s\n", string_status);
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
 * Handles the FUSE call to create a hard link.
 *
 * Returns 0 on success and an appropriate negative error code on failure.
 */
int nfs_link(const char *target_path, const char *link_path) {
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    // make copies of paths since dirname() and basename() modify arguments
    char *to_dir_copy = strdup(link_path);
    char *to_path_copy = strdup(link_path);

    LinkData link_data;
    link_data.target_path = discard_const(target_path);
    char *directory_path =
        dirname(to_dir_copy); // returns "." if there are no '/'s in the path (i.e. just file name 'file')
    link_data.containing_directory_path = strcmp(directory_path, ".") == 0 ? "/" : directory_path;
    link_data.link_name = basename(to_path_copy);

    callback_data.return_data = &link_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if (pthread_create(&blocking_thread, NULL, blocking_link, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

    wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    free(to_dir_copy);
    free(to_path_copy);

    return callback_data.error_code;
}
