#include "test_common.h"

/*
* Creates a deep copy of the given NfsFh__NfsFileHandle.
*/
NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle) {
    NfsFh__NfsFileHandle nfs_filehandle_copy = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle_copy.inode_number = nfs_filehandle->inode_number;
    nfs_filehandle_copy.timestamp = nfs_filehandle->timestamp;

    return nfs_filehandle_copy;
}

/*
* Returns a string describing the given Nfs__Stat.
*
* The user of this function takes the responsibility to free the returned string.
*/
char *nfs_stat_to_string(Nfs__Stat stat) {
    switch(stat) {
        case NFS__STAT__NFS_OK:
            return strdup("NFS_OK");
        case NFS__STAT__NFSERR_PERM:
            return strdup("NFSERR_PERM");
        case NFS__STAT__NFSERR_NOENT:
            return strdup("NFSERR_NOENT");
        case NFS__STAT__NFSERR_IO:
            return strdup("NFSERR_IO");
        case NFS__STAT__NFSERR_NXIO:
            return strdup("NFSERR_NXIO");
        case NFS__STAT__NFSERR_ACCES:
            return strdup("NFSERR_ACCES");
        case NFS__STAT__NFSERR_EXIST:
            return strdup("NFSERR_EXIST");
        case NFS__STAT__NFSERR_NODEV:
            return strdup("NFSERR_NODEV");
        case NFS__STAT__NFSERR_NOTDIR:
            return strdup("NFSERR_NOTDIR");
        case NFS__STAT__NFSERR_ISDIR:
            return strdup("NFSERR_ISDIR");
        case NFS__STAT__NFSERR_FBIG:
            return strdup("NFSERR_FBIG");
        case NFS__STAT__NFSERR_NOSPC:
            return strdup("NFSERR_NOSPC");
        case NFS__STAT__NFSERR_ROFS:
            return strdup("NFSERR_ROFS");
        case NFS__STAT__NFSERR_NAMETOOLONG:
            return strdup("NFSERR_NAMETOOLONG");
        case NFS__STAT__NFSERR_NOTEMPTY:
            return strdup("NFSERR_NOTEMPTY");
        case NFS__STAT__NFSERR_DQUOT:
            return strdup("NFSERR_DQUOT");
        case NFS__STAT__NFSERR_STALE:
            return strdup("NFSERR_STALE");
        case NFS__STAT__NFSERR_WFLUSH:
            return strdup("NFSERR_WFLUSH");
        default:
            return strdup("Unknown");
    }
}
