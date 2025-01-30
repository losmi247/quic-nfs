#include "handlers.h"

#define WRITE_BYTES_PER_RPC 5000

typedef struct WriteData {
    char *path;
    
	char *write_buffer;
	size_t write_buffer_size;
	off_t offset;

	size_t bytes_written;
} WriteData;

void *blocking_write(void *arg) {
    CallbackData *callback_data = (CallbackData *) arg;

    WriteData *write_data = (WriteData *) callback_data->return_data;

    pthread_mutex_lock(&nfs_mutex);

    Nfs__FType file_type;
    int error_code;
    Nfs__FHandle *file_fhandle = resolve_absolute_path(rpc_connection_context, filesystem_root_fhandle, write_data->path, &file_type, &error_code);
    if(file_fhandle == NULL) {
        printf("nfs_write: failed to resolve the path %s to a file\n", write_data->path);

        callback_data->error_code = -error_code;

        pthread_mutex_unlock(&nfs_mutex);

        goto signal;
    }

    write_data->bytes_written = 0;
    while(write_data->bytes_written < write_data->write_buffer_size) {
        Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
        writeargs.file = file_fhandle;
        writeargs.offset = write_data->offset + write_data->bytes_written;

        writeargs.nfsdata.data = write_data->write_buffer + write_data->bytes_written;
        size_t bytes_left_to_write = write_data->write_buffer_size - write_data->bytes_written;
        if(bytes_left_to_write < WRITE_BYTES_PER_RPC) {
            writeargs.nfsdata.len = bytes_left_to_write;
        }
        else {
            writeargs.nfsdata.len = WRITE_BYTES_PER_RPC;
        }

        writeargs.beginoffset = writeargs.totalcount = 0;   // unused fields

        Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
        int status = nfs_procedure_8_write_to_file(rpc_connection_context, writeargs, attrstat);
        if(status != 0) {
            free(attrstat);

            printf("Error: Invalid RPC reply received from the server with status %d\n", status);

            callback_data->error_code = -EIO;

            pthread_mutex_unlock(&nfs_mutex);

            goto signal;
        }

        if(validate_nfs_attr_stat(attrstat) > 0) {
            printf("Error: Invalid NFS READ procedure result received from the server\n");

            nfs__attr_stat__free_unpacked(attrstat, NULL);

            callback_data->error_code = -EIO;

            pthread_mutex_unlock(&nfs_mutex);

            goto signal;
        }

        if(attrstat->nfs_status->stat == NFS__STAT__NFSERR_ACCES) {
            printf("Error: Permission denied\n");
            
            nfs__attr_stat__free_unpacked(attrstat, NULL);

            callback_data->error_code = -EACCES;

            pthread_mutex_unlock(&nfs_mutex);

            goto signal;
        }
        else if(attrstat->nfs_status->stat != NFS__STAT__NFS_OK) {
            char *string_status = nfs_stat_to_string(attrstat->nfs_status->stat);
            printf("Error: Failed to write to a file in the current working directory with status %s\n", string_status);
            free(string_status);

            nfs__attr_stat__free_unpacked(attrstat, NULL);

            callback_data->error_code = map_nfs_error(attrstat->nfs_status->stat);

            pthread_mutex_unlock(&nfs_mutex);

            goto signal;
        }

        write_data->bytes_written += writeargs.nfsdata.len;

        nfs__attr_stat__free_unpacked(attrstat, NULL);
    }

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
* Handles the FUSE call to write to a file.
*
* Returns 0 on success and the appropriate negative error code on failure.
*/
int nfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	CallbackData callback_data;
    memset(&callback_data, 0, sizeof(CallbackData));
    callback_data.is_finished = 0;
    callback_data.error_code = 0;

	WriteData write_data;
    write_data.path = discard_const(path);
	
	write_data.write_buffer = discard_const(buffer);
	write_data.write_buffer_size = size;
	write_data.offset = offset;

    callback_data.return_data = &write_data;

    pthread_mutex_init(&callback_data.lock, NULL);
    pthread_cond_init(&callback_data.cond, NULL);

    pthread_t blocking_thread;
    if(pthread_create(&blocking_thread, NULL, blocking_write, &callback_data) != 0) {
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
		return write_data.bytes_written;
	}
}