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
* Creates a NFS filehandle for the file at the given absolute path. On successful exection, 
* it adds a mapping to the inode cache given in 'inode_number_cache' argument, to remember 
* what absolute path this file's inode number corresponds to.
*
* Returns 0 on success and > 0 on failure.
*
* The user of this function is responsible for deallocating the created NFS filehandle. This is
* done either by having it removed from the InodeCache at some point, or by the InodeCache clean up
* on server shutdown.
*/
NfsFh__NfsFileHandle *create_nfs_filehandle(char *absolute_path, InodeCache *inode_number_cache) {
    ino_t inode_number;
    int error_code = get_inode_number(absolute_path, &inode_number);
    if(error_code > 0) {
        // we couldn't get inode number of file/directory at this absolute path
        return NULL;
    }

    time_t unix_timestamp = time(NULL);

    NfsFh__NfsFileHandle *nfs_filehandle = malloc(sizeof(NfsFh__NfsFileHandle));
    if(nfs_filehandle == NULL) {
        return NULL;
    }
    nfs_fh__nfs_file_handle__init(nfs_filehandle);
    nfs_filehandle->inode_number = inode_number;
    nfs_filehandle->timestamp = unix_timestamp;

    // remember what absolute path this inode number corresponds to
    error_code = add_inode_mapping(nfs_filehandle, absolute_path, inode_number_cache);
    if(error_code > 0) {
        free(nfs_filehandle);
        return NULL;
    }
    
    return nfs_filehandle;
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
