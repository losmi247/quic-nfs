#include "file_management.h"

/*
* Given the absolute path of a directory or a file, places its inode number in 
* 'inode_number' argument. 
*
* Returns 0 on success, and > 0 on failure (no such file or directory usually).
*/
int get_inode_number(char *absolute_path, ino_t *inode_number) {
    struct stat file_stat;
    if(stat(absolute_path, &file_stat) < 0) {
        perror("Failed retrieving file stats");
        return 1;
    }

    *inode_number = file_stat.st_ino;

    return 0;
}

/*
* Given an initialized NfsFh__NfsFileHandle, fills its fields to make it a new filehandle for the given absolute path 
* (either a directory or a regular file).
* Then it adds a mapping to the inode cache to remember what absolute path the inode number corresponds to, using the 
* inode cache given in 'inode_number_cache' argument.
*
* Returns 0 on success and > 0 on failure.
*
* TODO (QNFS-19): concatenate to this a UNIX timestamp
*/
int create_nfs_filehandle(char *absolute_path, NfsFh__NfsFileHandle *nfs_filehandle, InodeCache *inode_number_cache) {
    if(absolute_path == NULL) {
        return 1;
    }

    ino_t inode_number;
    int error_code = get_inode_number(absolute_path, &inode_number);
    if(error_code > 0) {
        // we couldn't get inode number of file/directory at this absolute path - it doesn't exist
        return 2;
    }

    // remember what absolute path this inode number corresponds to
    add_inode_mapping(inode_number, absolute_path, inode_number_cache);

    nfs_filehandle->inode_number = inode_number;
    
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
* The user of this function takes the responsibility to free the heap-allocated TimeVal structures.
*/
int get_attributes(char *absolute_path, Nfs__FAttr *fattr) {
    struct stat file_stat;
    if(stat(absolute_path, &file_stat) < 0) {
        char *msg = malloc(sizeof(char) * 100);
        snprintf(msg, 100, "Failed retrieving file stats for file with absolute path '%s'", absolute_path);
        perror(msg);

        free(msg);

        return 1;
    }

    fattr->type = decode_file_type(file_stat.st_mode);

    fattr->mode = file_stat.st_mode;
    fattr->nlink = file_stat.st_nlink;
    fattr->uid = file_stat.st_uid;
    fattr->gid = file_stat.st_gid;
    fattr->size = file_stat.st_size;
    fattr->blocksize = file_stat.st_blksize;

    fattr->rdev = file_stat.st_rdev;
    fattr->blocks = file_stat.st_blocks;
    fattr->fsid = file_stat.st_dev;
    fattr->fileid = file_stat.st_ino;

    Nfs__TimeVal *atime = malloc(sizeof(Nfs__TimeVal));
    nfs__time_val__init(atime);
    atime->seconds = file_stat.st_atim.tv_sec;
    atime->useconds = file_stat.st_atim.tv_nsec;
    Nfs__TimeVal *mtime = malloc(sizeof(Nfs__TimeVal));
    nfs__time_val__init(mtime);
    mtime->seconds = file_stat.st_mtim.tv_sec;
    mtime->useconds = file_stat.st_mtim.tv_nsec;
    Nfs__TimeVal *ctime = malloc(sizeof(Nfs__TimeVal));
    nfs__time_val__init(ctime);
    ctime->seconds = file_stat.st_ctim.tv_sec;
    ctime->useconds = file_stat.st_ctim.tv_nsec;

    fattr->atime = atime;
    fattr->mtime = mtime;
    fattr->ctime = ctime;

    return 0;
}