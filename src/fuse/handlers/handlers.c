#include "handlers.h"

pthread_mutex_t nfs_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
* Waits on a condition variable that is signalled once the
* NFS server returns a reply.
*/
void wait_for_nfs_reply(CallbackData *callback_data) {
    pthread_mutex_lock(&callback_data->lock);

    while(!callback_data->is_finished) {
        pthread_cond_wait(&callback_data->cond, &callback_data->lock);  // releases the lock while waiting
    }

    pthread_mutex_unlock(&callback_data->lock);
}

/*
* NFS error codes in RFC 1094 and Linux error codes can sometimes mismatch
* (e.g. NFSERR_NOTEMPTY is 66, but ENOTEMPTY is 39 on Linux) so we have to
* map error codes.
*/
int map_nfs_error(int nfs_status) {
    switch (nfs_status) {
        case NFS__STAT__NFS_OK: return 0;
        case NFS__STAT__NFSERR_PERM: return -EPERM;
        case NFS__STAT__NFSERR_NOENT: return -ENOENT;
        case NFS__STAT__NFSERR_IO: return -EIO;
        case NFS__STAT__NFSERR_NXIO: return ENXIO;
        case NFS__STAT__NFSERR_ACCES: return -EACCES;
        case NFS__STAT__NFSERR_EXIST: return -EEXIST;
        case NFS__STAT__NFSERR_NODEV: return -ENODEV;
        case NFS__STAT__NFSERR_NOTDIR: return -ENOTDIR;
        case NFS__STAT__NFSERR_ISDIR: return -EISDIR;
        case NFS__STAT__NFSERR_FBIG: return -EFBIG;
        case NFS__STAT__NFSERR_NOSPC: return -ENOSPC;
        case NFS__STAT__NFSERR_ROFS: return -EROFS;
        case NFS__STAT__NFSERR_NAMETOOLONG: return -ENAMETOOLONG;
        case NFS__STAT__NFSERR_NOTEMPTY: return -ENOTEMPTY;
        case NFS__STAT__NFSERR_DQUOT: return -EDQUOT;
        case NFS__STAT__NFSERR_STALE: return -ESTALE;
        case NFS__STAT__NFSERR_WFLUSH: return -EIO;
        default: 
            return -EIO;  // default to I/O error for unknown cases
    }
}
