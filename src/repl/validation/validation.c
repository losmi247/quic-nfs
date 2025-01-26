#include "validation.h"

/*
* Validates the structure of the given FHandle.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_mount_fhandle(Mount__FHandle *fhandle) {
    if(fhandle == NULL) {
        return 1;
    }

    if(fhandle->nfs_filehandle == NULL) {
        return 1;
    }

    return 0;
}

/*
* Validates the structure of the given FHandle.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_fhandle(Nfs__FHandle *fhandle) {
    if(fhandle == NULL) {
        return 1;
    }

    if(fhandle->nfs_filehandle == NULL) {
        return 1;
    }

    return 0;
}

/*
* Validates the structure of the given FhStatus.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_mount_fh_status(Mount__FhStatus *fh_status) {
    if(fh_status == NULL) {
        return 1;
    }

    if(fh_status->mnt_status == NULL) {
        return 1;
    }

    if(fh_status->mnt_status->stat == MOUNT__STAT__MNT_OK) {
        if(fh_status->fhstatus_body_case != MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY) {
            return 1;
        }
        if(validate_mount_fhandle(fh_status->directory) > 0) {
            return 1;
        }
    }
    else {
        if(fh_status->fhstatus_body_case != MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE) {
            return 1;
        }
        if(fh_status->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}

/*
* Validates the structure of the given NfsStat.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_nfs_stat(Nfs__NfsStat *nfsstat) {
    if(nfsstat == NULL) {
        return 1;
    }

    return 0;
}

/*
* Validates the structure of the given ReadDirOk.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_read_dir_ok(Nfs__ReadDirOk *readdirok) {
    Nfs__DirectoryEntriesList *entries = readdirok->entries;
    while(entries != NULL) {
        if(entries->name == NULL || entries->name->filename == NULL) {
            return 1;
        }
        
        if(entries->cookie == NULL) {
            return 1;
        }

        entries = entries->nextentry;
    }

    return 0;
}

/*
* Validates the structure of the given ReadDirRes.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_read_dir_res(Nfs__ReadDirRes *readdirres) {
    if(readdirres == NULL) {
        return 1;
    }

    if(readdirres->nfs_status == NULL) {
        return 1;
    }

    if(readdirres->nfs_status->stat == NFS__STAT__NFS_OK) {
        if(readdirres->body_case != NFS__READ_DIR_RES__BODY_READDIROK) {
            return 1;
        }
        if(validate_nfs_read_dir_ok(readdirres->readdirok) > 0) {
            return 1;
        }
    }
    else {
        if(readdirres->body_case != NFS__READ_DIR_RES__BODY_DEFAULT_CASE) {
            return 1;
        }
        if(readdirres->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}

/*
* Validates the structure of the given FAttr.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_fattr(Nfs__FAttr *fattr) {
    if(fattr == NULL) {
        return 1;
    }

    if(fattr->nfs_ftype == NULL || fattr->atime == NULL || fattr->mtime == NULL || fattr->ctime == NULL) {
        return 1;
    }

    return 0;
}

/*
* Validates the structure of the given AttrStat.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_attr_stat(Nfs__AttrStat *attrstat) {
    if(attrstat == NULL) {
        return 1;
    }

    if(attrstat->nfs_status == NULL) {
        return 1;
    }

    if(attrstat->nfs_status->stat == NFS__STAT__NFS_OK) {
        if(attrstat->body_case != NFS__ATTR_STAT__BODY_ATTRIBUTES) {
            return 1;
        }
        if(validate_nfs_fattr(attrstat->attributes) > 0) {
            return 1;
        }
    }
    else {
        if(attrstat->body_case != NFS__ATTR_STAT__BODY_DEFAULT_CASE) {
            return 1;
        }
        if(attrstat->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}

/*
* Validates the structure of the given DirOpOk.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_dir_op_ok(Nfs__DirOpOk *diropok) {
    if(diropok == NULL) {
        return 1;
    }

    if(diropok->file == NULL) {
        return 1;
    }

    if(validate_nfs_fhandle(diropok->file) > 0) {
        return 1;
    }

    if(validate_nfs_fattr(diropok->attributes) > 0) {
        return 1;
    }

    return 0;
}

/*
* Validates the structure of the given DirOpRes.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_dir_op_res(Nfs__DirOpRes *diropres) {
    if(diropres == NULL) {
        return 1;
    }

    if(diropres->nfs_status == NULL) {
        return 1;
    }

    if(diropres->nfs_status->stat == NFS__STAT__NFS_OK) {
        if(diropres->body_case != NFS__DIR_OP_RES__BODY_DIROPOK) {
            return 1;
        }
        if(validate_nfs_dir_op_ok(diropres->diropok) > 0) {
            return 1;
        }
    }
    else {
        if(diropres->body_case != NFS__DIR_OP_RES__BODY_DEFAULT_CASE) {
            return 1;
        }
        if(diropres->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}

/*
* Validates the structure of the given ReadRes.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_read_res(Nfs__ReadRes *readres) {
    if(readres == NULL) {
        return 1;
    }

    if(readres->nfs_status == NULL) {
        return 1;
    }

    if(readres->nfs_status->stat == NFS__STAT__NFS_OK) {
        if(readres->body_case != NFS__READ_RES__BODY_READRESBODY) {
            return 1;
        }

        Nfs__ReadResBody *readresbody = readres->readresbody;
        if(readresbody == NULL) {
            return 1;
        }

        if(validate_nfs_fattr(readresbody->attributes)) {
            return 1;
        }
        if(readresbody->nfsdata.len > 0 && readresbody->nfsdata.data == NULL) {
            return 1;
        }
        if(readresbody->nfsdata.len == 0 && readresbody->nfsdata.data != NULL) {
            return 1;
        }
    }
    else {
        if(readres->body_case != NFS__READ_RES__BODY_DEFAULT_CASE) {
            return 1;
        }
        if(readres->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}

/*
* Validates the structure of the given ReadLinkRes.
*
* Returns 0 on success and > 0 on failure.
*/
int validate_nfs_read_link_res(Nfs__ReadLinkRes *readlinkres) {
    if(readlinkres == NULL) {
        return 1;
    }

    if(readlinkres->nfs_status == NULL) {
        return 1;
    }

    if(readlinkres->nfs_status->stat == NFS__STAT__NFS_OK) {
        if(readlinkres->body_case != NFS__READ_LINK_RES__BODY_DATA) {
            return 1;
        }

        Nfs__Path *path = readlinkres->data;
        if(path == NULL) {
            return 1;
        }

        if(path->path == NULL) {
            return 1;
        }
    }
    else {
        if(readlinkres->body_case != NFS__READ_LINK_RES__BODY_DEFAULT_CASE) {
            return 1;
        }
        if(readlinkres->default_case == NULL) {
            return 1;
        }
    }

    return 0;
}