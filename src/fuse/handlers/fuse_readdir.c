#include "handlers.h"

#define READDIR_BYTES_PER_RPC 50

typedef struct ReaddirData {
    char *path;

    void *buffer;
    fuse_fill_dir_t filler;
} ReaddirData;

typedef struct FileNamesList {
    char *filename;
    struct FileNamesList *next;
} FileNamesList;

void *blocking_readdir(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    ReaddirData *readdir_data = (ReaddirData *) callback_data->return_data;

    void *buffer = readdir_data->buffer;
    fuse_fill_dir_t filler = readdir_data->filler;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, readdir_data->path, &file_type);
    if(file_fhandle == NULL) {
        printf("nfs_readdir: failed to resolve the path %s to a file\n", readdir_data->path);

        callback_data->error_code = -ENOENT;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    FileNamesList *filenames_list_head = NULL;
    FileNamesList *filenames_list_tail = NULL;

    uint64_t offset_cookie = 0;
    int read_all_directory_entries = 0;
    while(!read_all_directory_entries) {
        Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
        nfs_cookie.value = offset_cookie;

        Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
        readdirargs.dir = file_fhandle;
        readdirargs.cookie = &nfs_cookie;
        readdirargs.count = READDIR_BYTES_PER_RPC;

        Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
        int status = nfs_procedure_16_read_from_directory(rpc_connection_context, readdirargs, readdirres);
        if(status != 0) {
            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            callback_data->error_code = -EIO;

            pthread_mutex_unlock(&nfs_mutex);

            free(readdirres);

            goto signal;
        }

        if(validate_nfs_read_dir_res(readdirres) > 0) {
            printf("Error: Invalid NFS procedure result received from the server\n");

            callback_data->error_code = -EIO;

            pthread_mutex_unlock(&nfs_mutex);

            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            goto signal;
        }

        if(readdirres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("Error: Permission denied\n");

            callback_data->error_code = -EACCES;

            pthread_mutex_unlock(&nfs_mutex);
            
            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            goto signal;
        }
        else if(readdirres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(readdirres->nfs_status->stat);
            printf("Error: Failed to read directory entries with status %s\n", string_status);
            free(string_status);

            callback_data->error_code = -(readdirres->nfs_status->stat);

            pthread_mutex_unlock(&nfs_mutex);

            nfs__read_dir_res__free_unpacked(readdirres, NULL);

            goto signal;
        }

        // remember all found directory entries
        Nfs__DirectoryEntriesList *entries = readdirres->readdirok->entries;
        while(entries != NULL) {
            // add the filename to the end of the list
            FileNamesList *new_filenames_list_entry = malloc(sizeof(FileNamesList));
            new_filenames_list_entry->filename = strdup(entries->name->filename);
            new_filenames_list_entry->next = NULL;
            if(filenames_list_tail == NULL) {
                filenames_list_head = filenames_list_tail = new_filenames_list_entry;
            }
            else {
                filenames_list_tail->next = new_filenames_list_entry;
                filenames_list_tail = new_filenames_list_entry;
            }

            // update the UNIX directory stream offset cookie
            offset_cookie = entries->cookie->value;

            entries = entries->nextentry;
        }

        if(readdirres->readdirok->eof) {
            read_all_directory_entries = 1;
        }

        nfs__read_dir_res__free_unpacked(readdirres, NULL);
    }

    pthread_mutex_unlock(&nfs_mutex);

    // give all file names in this directory
    FileNamesList *filenames_list = filenames_list_head;
    while(filenames_list != NULL) {
        filler(buffer, filenames_list->filename, NULL, 0 ,0);   // modify the NULL,0,0 args to give entry attributes now

        filenames_list = filenames_list->next;
    }

    // clean up the filenames list
    while(filenames_list_head != NULL) {
        FileNamesList *next = filenames_list_head->next;

        free(filenames_list_head->filename);
        free(filenames_list_head);

        filenames_list_head = next;
    }

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

int nfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    printf("path is %s\n", path); fflush(stdout);
    CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

    ReaddirData readdir_data;
    readdir_data.path = path;
	readdir_data.buffer = buffer;
    readdir_data.filler = filler;

    callback_data.return_data = &readdir_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_readdir, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

	wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

    return callback_data.error_code;
}