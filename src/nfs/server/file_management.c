#include "file_management.h"

/*
* Given the absolute path of a directory or a file, places its inode number in 
* 'inode_number' argument. 
*
* Returns 0 on success, and > 0 on failure.
*/
int get_inode_number(char *absolute_path, ino_t *inode_number) {
    struct stat file_stat;
    if(lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return 1;
    }

    *inode_number = file_stat.st_ino;

    return 0;
}

/*
* Given an initialized NfsFh__NfsFileHandle, fills its fields to make it a new filehandle for the given absolute path 
* (either a directory or a regular file).
*
* On successful exection, it adds a mapping to the inode cache given in 'inode_number_cache' argument, to remember 
* what absolute path this file's inode number corresponds to.
*
* Returns 0 on success and > 0 on failure.
*/
int create_nfs_filehandle(char *absolute_path, NfsFh__NfsFileHandle *nfs_filehandle, InodeCache *inode_number_cache) {
    ino_t inode_number;
    int error_code = get_inode_number(absolute_path, &inode_number);
    if(error_code > 0) {
        // we couldn't get inode number of file/directory at this absolute path
        return 1;
    }

    // remember what absolute path this inode number corresponds to
    add_inode_mapping(inode_number, absolute_path, inode_number_cache);

    time_t unix_timestamp = time(NULL);

    nfs_filehandle->inode_number = inode_number;
    nfs_filehandle->timestamp = unix_timestamp;
    
    return 0;
}

/*
* Reads out the file type from the mode.
*/
Nfs__FType decode_file_type(mode_t st_mode) {
    if(S_ISREG(st_mode)) {
        return NFS__FTYPE__NFREG;
    }
    if(S_ISDIR(st_mode)) {
        return NFS__FTYPE__NFDIR;
    }
    if(S_ISBLK(st_mode)) {
        return NFS__FTYPE__NFBLK;
    }
    if(S_ISCHR(st_mode)) {
        return NFS__FTYPE__NFCHR;
    }
    if(S_ISLNK(st_mode)) {
        return NFS__FTYPE__NFLNK;
    }

    return NFS__FTYPE__NFNON;
}

/*
* Given an absolute path of a file or a directory, gives the corresponding file's attributes in 'fattr'.
* Returns 0 on succes and > 0 on failure.
*
* The user of this function takes the responsibility to free the heap-allocated NfsFType and TimeVal structures.
*/
int get_attributes(char *absolute_path, Nfs__FAttr *fattr) {
    struct stat file_stat;
    if(lstat(absolute_path, &file_stat) < 0) {
        perror_msg("Failed retrieving file stats for file/directory at absolute path %s", absolute_path);
        return 1;
    }

    Nfs__NfsFType *nfs_ftype = malloc(sizeof(Nfs__NfsFType));
    if(nfs_ftype == NULL) {
        perror("Failed to allocate 'NfsFType'");
        return 2;
    }
    nfs__nfs_ftype__init(nfs_ftype);
    nfs_ftype->ftype = decode_file_type(file_stat.st_mode);
    fattr->nfs_ftype = nfs_ftype;

    fattr->mode = file_stat.st_mode;
    fattr->nlink = file_stat.st_nlink;
    fattr->uid = file_stat.st_uid;
    fattr->gid = file_stat.st_gid;
    fattr->size = file_stat.st_size;
    fattr->blocksize = file_stat.st_blksize;

    fattr->rdev = file_stat.st_rdev;
    fattr->blocks = file_stat.st_blocks;
    fattr->fsid = file_stat.st_dev;
    fattr->fileid = file_stat.st_ino;  // we use file's inode number as fileid (unique identifier on this device)

    Nfs__TimeVal *atime = malloc(sizeof(Nfs__TimeVal));
    if(atime == NULL) {
        perror("Failed to allocate 'TimeVal'");

        free(nfs_ftype);

        return 3;
    }
    nfs__time_val__init(atime);
    atime->seconds = file_stat.st_atim.tv_sec;
    atime->useconds = file_stat.st_atim.tv_nsec;

    Nfs__TimeVal *mtime = malloc(sizeof(Nfs__TimeVal));
    if(mtime == NULL) {
        perror("Failed to allocate 'TimeVal'");

        free(nfs_ftype);
        free(atime);

        return 3;
    }
    nfs__time_val__init(mtime);
    mtime->seconds = file_stat.st_mtim.tv_sec;
    mtime->useconds = file_stat.st_mtim.tv_nsec;

    Nfs__TimeVal *ctime = malloc(sizeof(Nfs__TimeVal));
    if(ctime == NULL) {
        perror("Failed to allocate 'TimeVal'");
        
        free(nfs_ftype);
        free(atime);
        free(mtime);

        return 3;
    }
    nfs__time_val__init(ctime);
    ctime->seconds = file_stat.st_ctim.tv_sec;
    ctime->useconds = file_stat.st_ctim.tv_nsec;

    fattr->atime = atime;
    fattr->mtime = mtime;
    fattr->ctime = ctime;

    return 0;
}

/*
* Deallocates heap allocated fields of the given FAttr. Does nothing if 'fattr' is NULL.
*/
void clean_up_fattr(Nfs__FAttr *fattr) {
    if(fattr == NULL) {
        return;
    }

    free(fattr->nfs_ftype);
    free(fattr->atime);
    free(fattr->mtime);
    free(fattr->ctime);
}

/*
* Reads up to 'byte_count' bytes from 'offset' in the file given by the absolute path, and places the
* result into 'destination_buffer' (must be allocated at least 'byte_count' bytes) and
* puts the number of bytes read into 'bytes_read'.
*
* Returns 0 on success and > 0 on failure.
*
* TODO (QNFS-25): make this function atomic, for use by multithreaded server
*/
int read_from_file(char *file_absolute_path, off_t offset, size_t byte_count, uint8_t *destination_buffer, size_t *bytes_read) {
    if (file_absolute_path == NULL) {
        return 1;
    }

    FILE *file = fopen(file_absolute_path, "rb");
    if(file == NULL) {
        perror_msg("Failed to open file at absolute path '%s' for reading", file_absolute_path);
        return 2;
    }

    if(fseek(file, offset, SEEK_SET) < 0) {
        perror_msg("Failed to seek file at absolute path '%s' at offset %ld", file_absolute_path, offset);

        fclose(file);
        
        return 3;
    }

    // read up to 'byte_count' bytes
    *bytes_read = fread(destination_buffer, 1, byte_count, file);
    // fread doesn't distinguish between error and end-of-file so we need to check
    if(ferror(file)) {
        fclose(file);
        return 4;
    }

    fclose(file);

    return 0;
}

/*
* Writes 'byte_count' bytes from 'offset' in the file given by the absolute path.
*
* Returns 0 on success and > 0 on failure.
*
* TODO (QNFS-25): make this function atomic, for use by multithreaded server
*/
int write_to_file(char *file_absolute_path, off_t offset, size_t byte_count, uint8_t *source_buffer) {
    if(file_absolute_path == NULL) {
        return 1;
    }

    FILE *file = fopen(file_absolute_path, "r+b");
    if(file == NULL) {
        perror_msg("Failed to open file at absolute path '%s' for writing", file_absolute_path);
        return 2;
    }

    if(fseek(file, offset, SEEK_SET) < 0) {
        perror_msg("Failed to seek file at absolute path '%s' at offset %ld", file_absolute_path, offset);

        fclose(file);

        return 3;
    }

    // write 'byte_count' bytes from the source buffer to the file
    size_t bytes_written = fwrite(source_buffer, 1, byte_count, file);
    if (bytes_written != byte_count) {
        perror_msg("Failed to write %d bytes to file at absolute path '%s', only wrote %d bytes", byte_count, file_absolute_path, bytes_written);
        
        fclose(file);

        switch(errno) {
            case EFBIG:     // attempted write that exceeds file size limits
                return 4;
            case EIO:       // physical IO error
                return 5;
            case ENOSPC:    // no space left on device
            case ENOMEM:
                return 6;
            default:
                return 7;
        }
    }

    fclose(file);
    return 0;
}

/*
* Given a client (by its AUTH_SYS parameters) and a directory by its absolute path and inode number,
* fetches the ReadDir session for this client and directory (or creates a new one if no such session is
* currently active) from the given collection of active ReadDir sessions ('active_read_dir_sessions' argument).
* Then reads directory entries from it, creates a list of them, and places that list in the 'head' argument. 
* It will read as many directory entries as possible, such that their total size does not exceed 'byte_count'.
* In case end of directory stream was reached, 'end_of_stream' is set to 1.
*
* Returns 0 on success and > 0 on failure. The 'directory_absolute_path' is only used for printing in
* case of an error.
*
* In case of successful execution, the user of this function takes the responsibility to free all 
* directory entries, Nfs__FileName's, Nfs__FileName.filename's, and Nfs__NfsCookie's inside them
* using the clean_up_directory_entries_list() function.
*/
int read_from_directory(Rpc__AuthSysParams *client_authsysparams, char *directory_absolute_path, ino_t directory_inode_number, ReadDirSessionsList **active_readdir_sessions, long offset_cookie, size_t byte_count, Nfs__DirectoryEntriesList **head, int *end_of_stream) {
    if(active_readdir_sessions == NULL) {
        fprintf(stderr, "read_from_directory: readdir_sessions is NULL\n");
        return 1;
    }

    if(client_authsysparams == NULL) {
        fprintf(stderr, "read_from_directory: client AuthSysParams is NULL\n");
        return 2;
    }
    
    if(find_readdir_session(client_authsysparams, directory_inode_number, *active_readdir_sessions) != 0) {
        // there is no active ReadDir session for this client and directory, so create one
        int error_code = add_new_readdir_session(client_authsysparams, directory_absolute_path, directory_inode_number, active_readdir_sessions);
        if(error_code > 0) {
            fprintf(stderr, "read_from_directory: failed to create a new ReadDir session\n");
            return 3;
        }
    }

    // get the ReadDir session for this client and directory
    ReadDirSession *readdir_session = get_readdir_session(client_authsysparams, directory_inode_number, *active_readdir_sessions);
    if(readdir_session == NULL) {
        fprintf(stderr, "read_from_directory: ReadDir session for client %s:%d:%d and directory %s not found\n", client_authsysparams->machinename, client_authsysparams->uid, client_authsysparams->gid, directory_absolute_path);
        return 4;
    }

    // 0 is a special cookie value meaning client wants to start from beginning of directory stream
    if(offset_cookie == 0) {
        rewinddir(readdir_session->directory_stream);
    }
    else {
        // set the position within directory stream to the specified cookie
        seekdir(readdir_session->directory_stream, offset_cookie);
    }

    uint32_t total_size = 0;
    Nfs__DirectoryEntriesList *directory_entries_list_head = NULL, *directory_entries_list_tail = NULL;
    do {
        errno = 0;
        struct dirent *directory_entry = readdir(readdir_session->directory_stream); // man page of 'readdir' says not to free this
        if(directory_entry == NULL) {
            if(errno == 0) {
                // end of the directory stream reached
                *end_of_stream = 1;
                break;
            }
            else{
                perror_msg("Error occured while reading entries of directory at absolute path %s", directory_absolute_path);

                // clean up the directory entries allocated so far
                clean_up_directory_entries_list(directory_entries_list_head);

                return 5;
            }
        }

        // construct a new directory entry
        Nfs__DirectoryEntriesList *new_directory_entry = malloc(sizeof(Nfs__DirectoryEntriesList));
        nfs__directory_entries_list__init(new_directory_entry);
        new_directory_entry->fileid = directory_entry->d_ino;  // fileid in FAttr is inode number, so this fileid should also be inode number

        Nfs__FileName *file_name = malloc(sizeof(Nfs__FileName));
        nfs__file_name__init(file_name);
        file_name->filename = malloc(sizeof(char) * NFS_MAXNAMLEN); // make a copy of the file name to persist after this dirent is deallocated by closedir()
        strcpy(file_name->filename, directory_entry->d_name);
        new_directory_entry->name = file_name;

        long posix_cookie = telldir(readdir_session->directory_stream);
        if(posix_cookie < 0) {
            perror_msg("Failed getting current location in directory stream of directory at absolute path %s", directory_absolute_path);

            // clean up the directory entries allocated so far
            clean_up_directory_entries_list(directory_entries_list_head);

            return 6;
        }
        Nfs__NfsCookie *nfs_cookie = malloc(sizeof(Nfs__NfsCookie));
        nfs__nfs_cookie__init(nfs_cookie);
        nfs_cookie->value = posix_cookie;
        new_directory_entry->cookie = nfs_cookie;

        new_directory_entry->nextentry = NULL;

        // check we're not exceeding limit on bytes read, using Protobuf get_packed_size
        size_t directory_entry_packed_size = nfs__directory_entries_list__get_packed_size(new_directory_entry);
        if(total_size + directory_entry_packed_size > byte_count) {
            break;
        }
        total_size += directory_entry_packed_size;

        // append the new directory entry to the end of the list
        if(directory_entries_list_tail == NULL) {
            directory_entries_list_head = directory_entries_list_tail = new_directory_entry;
            // set 'head' argument to head of the list of entries
            *head = directory_entries_list_head;
        }
        else {
            directory_entries_list_tail->nextentry = new_directory_entry;
            directory_entries_list_tail = new_directory_entry;
        }
    } while(total_size < byte_count);

    // update the time of the last read from this directory stream
    readdir_session->last_readdir_rpc_timestamp = time(NULL);

    // if the stream was read to the end, close it
    if(*end_of_stream == 1) {
        int error_code = remove_readdir_session(client_authsysparams, directory_inode_number, active_readdir_sessions);
        if(error_code > 0) {
            fprintf(stderr, "read_from_directory: failed removing the ReadDir session client %s:%d:%d and directory %s\n", client_authsysparams->machinename, client_authsysparams->uid, client_authsysparams->gid, directory_absolute_path);

            // clean up the directory entries allocated so far
            clean_up_directory_entries_list(directory_entries_list_head);

            return 7;
        }
    }

    return 0;
}

void clean_up_directory_entries_list(Nfs__DirectoryEntriesList *directory_entries_list_head) {
    while(directory_entries_list_head != NULL) {
        Nfs__DirectoryEntriesList *next = directory_entries_list_head->nextentry;

        free(directory_entries_list_head->name->filename);
        free(directory_entries_list_head->name);
        free(directory_entries_list_head->cookie);

        free(directory_entries_list_head);

        directory_entries_list_head = next;
    }
}