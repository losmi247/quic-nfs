#include "filehandle_management.h"

/*
* Creates a deep copy of the given NfsFh__NfsFileHandle.
*/
NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle) {
    NfsFh__NfsFileHandle nfs_filehandle_copy = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle_copy.inode_number = nfs_filehandle->inode_number;
    nfs_filehandle_copy.timestamp = nfs_filehandle->timestamp;

    return nfs_filehandle_copy;
}