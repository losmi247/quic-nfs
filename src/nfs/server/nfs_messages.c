#include "nfs_messages.h"

/*
* Takes a Nfs__Stat and if it's NFS__STAT__NFS_OK, creates an AttrStat message
* with default case and that status.
*
* If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
*
* The user of this fuction takes the responsibility to free the AttrStat, NfsStat,
* and Empty allocated in this function.
*/
Nfs__AttrStat *create_default_case_attr_stat(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    nfs__attr_stat__init(attr_stat);

    Nfs__NfsStat *nfs_status = malloc(sizeof(Nfs__NfsStat));
    nfs__nfs_stat__init(nfs_status);
    nfs_status->stat = non_nfs_ok_status;
    attr_stat->nfs_status = nfs_status;
    attr_stat->body_case = NFS__ATTR_STAT__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    attr_stat->default_case = empty;

    return attr_stat;
}

/*
* Takes a Nfs__Stat and if it's NFS__STAT__NFS_OK, creates an DirOpRes message
* with default case and that status.
*
* If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
*
* The user of this fuction takes the responsibility to free the DirOpRes, NfsStat,
* and Empty allocated in this function.
*/
Nfs__DirOpRes *create_default_case_dir_op_res(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    nfs__dir_op_res__init(diropres);

    Nfs__NfsStat *nfs_status = malloc(sizeof(Nfs__NfsStat));
    nfs__nfs_stat__init(nfs_status);
    nfs_status->stat = non_nfs_ok_status;
    diropres->nfs_status = nfs_status;
    diropres->body_case = NFS__DIR_OP_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    diropres->default_case = empty;

    return diropres;
}

/*
* Takes a Nfs__Stat and if it's NFS__STAT__NFS_OK, creates an ReadRes message
* with default case and that status.
*
* If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
*
* The user of this fuction takes the responsibility to free the ReadRes, NfsStat,
* and Empty allocated in this function.
*/
Nfs__ReadRes *create_default_case_read_res(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    nfs__read_res__init(readres);

    Nfs__NfsStat *nfs_status = malloc(sizeof(Nfs__NfsStat));
    nfs__nfs_stat__init(nfs_status);
    nfs_status->stat = non_nfs_ok_status;
    readres->nfs_status = nfs_status;
    readres->body_case = NFS__READ_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readres->default_case = empty;

    return readres;
}

/*
* Takes a Nfs__Stat and if it's NFS__STAT__NFS_OK, creates an ReadDirRes message
* with default case and that status.
*
* If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
*
* The user of this fuction takes the responsibility to free the ReadDirRes, NfsStat,
* and Empty allocated in this function.
*/
Nfs__ReadDirRes *create_default_case_read_dir_res(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    nfs__read_dir_res__init(readdirres);

    Nfs__NfsStat *nfs_status = malloc(sizeof(Nfs__NfsStat));
    nfs__nfs_stat__init(nfs_status);
    nfs_status->stat = non_nfs_ok_status;
    readdirres->nfs_status = nfs_status;
    readdirres->body_case = NFS__READ_DIR_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readdirres->default_case = empty;

    return readdirres;
}