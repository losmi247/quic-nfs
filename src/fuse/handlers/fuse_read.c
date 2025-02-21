#include "handlers.h"

#define READ_BYTES_PER_RPC 5000

typedef struct ReadData {
    char *path;
    
	char *read_buffer;
	size_t bytes_to_read;
	off_t offset;

	size_t bytes_read;
} ReadData;

void *blocking_read(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    ReadData *read_data = (ReadData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, read_data->path, &file_type, &error_code);
    if(file_fhandle == NULL) {
        printf("nfs_read: failed to resolve the path %s to a file\n", read_data->path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

	read_data->bytes_read = 0;
    uint64_t file_size = READ_BYTES_PER_RPC;    // we will know the file size after the first READ RPC
    do {
        Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
        readargs.file = file_fhandle;
        readargs.offset = read_data->offset + read_data->bytes_read;

		size_t bytes_left_to_read = read_data->bytes_to_read - read_data->bytes_read;
		if(bytes_left_to_read < READ_BYTES_PER_RPC) {
        	readargs.count = bytes_left_to_read;
		}
		else {
			readargs.count = READ_BYTES_PER_RPC;
		}

        readargs.totalcount = 0;    // unused field

        Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
        int status = nfs_procedure_6_read_from_file(rpc_connection_context, readargs, readres);
        if(status != 0) {
            free(readres);
            free(read_data);

            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            callback_data->error_code = -EIO;

			pthread_mutex_unlock(&nfs_mutex);

			goto signal;
        }

        if(validate_nfs_read_res(readres) > 0) {
            printf("Error: Invalid NFS READ procedure result received from the server\n");

            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

			callback_data->error_code = -EIO;

			pthread_mutex_unlock(&nfs_mutex);

			goto signal;
        }

        if(readres->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("cat: Permission denied\n");

            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

            callback_data->error_code = -EACCES;

			pthread_mutex_unlock(&nfs_mutex);

			goto signal;
        }
        else if(readres->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(readres->nfs_status->stat);
            printf("Error: Failed to read a file in the current working directory with status %s\n", string_status);
            free(string_status);

            nfs__read_res__free_unpacked(readres, NULL);
            free(read_data);

			callback_data->error_code = map_nfs_error(readres->nfs_status->stat);

			pthread_mutex_unlock(&nfs_mutex);

			goto signal;
        }

		// save the read data
		size_t bytes_to_save = readres->readresbody->nfsdata.len <= bytes_left_to_read ? readres->readresbody->nfsdata.len : bytes_left_to_read;
		memcpy(read_data->read_buffer + read_data->bytes_read, readres->readresbody->nfsdata.data, bytes_to_save);

        file_size = readres->readresbody->attributes->size;
        read_data->bytes_read += bytes_to_save;

        nfs__read_res__free_unpacked(readres, NULL);
    } while(read_data->bytes_read < read_data->bytes_to_read && read_data->offset + read_data->bytes_read < file_size);

    pthread_mutex_unlock(&nfs_mutex);

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
* Handles the FUSE call to read from a file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

	ReadData read_data;
    read_data.path = discard_const(path);
	
	read_data.read_buffer = buffer;
	read_data.bytes_to_read = size;
	read_data.offset = offset;

    callback_data.return_data = &read_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_read, &callback_data) != 0) {
        return -EIO;
    }

    pthread_detach(blocking_thread);

	wait_for_nfs_reply(&callback_data);

    pthread_mutex_destroy(&callback_data.lock);
    pthread_cond_destroy(&callback_data.cond);

	if(callback_data.error_code != 0) {
		return callback_data.error_code;
	}
	else {
		return read_data.bytes_read;
	}
}
