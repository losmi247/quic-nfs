#include "nfs_messages.h"

/*
 * Creates a NfsStat structure with the given status.
 *
 * The user of this fuction takes the responsibility to free the NfsStat allocated
 * in this function.
 */
Nfs__NfsStat *create_nfs_stat(Nfs__Stat stat) {
    Nfs__NfsStat *nfs_status = malloc(sizeof(Nfs__NfsStat));
    nfs__nfs_stat__init(nfs_status);
    nfs_status->stat = stat;

    return nfs_status;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates an AttrStat message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the AttrStat, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__AttrStat *create_default_case_attr_stat(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    nfs__attr_stat__init(attr_stat);

    attr_stat->nfs_status = create_nfs_stat(non_nfs_ok_status);
    attr_stat->body_case = NFS__ATTR_STAT__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    attr_stat->default_case = empty;

    return attr_stat;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates a DiOpRes message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the DirOpRes, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__DirOpRes *create_default_case_dir_op_res(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    nfs__dir_op_res__init(diropres);

    diropres->nfs_status = create_nfs_stat(non_nfs_ok_status);
    diropres->body_case = NFS__DIR_OP_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    diropres->default_case = empty;

    return diropres;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates a ReadLinkRes message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the ReadLinkRes, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__ReadLinkRes *create_default_case_read_link_res(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadLinkRes *readlinkres = malloc(sizeof(Nfs__ReadLinkRes));
    nfs__read_link_res__init(readlinkres);

    readlinkres->nfs_status = create_nfs_stat(non_nfs_ok_status);
    readlinkres->body_case = NFS__READ_LINK_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readlinkres->default_case = empty;

    return readlinkres;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates a ReadRes message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the ReadRes, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__ReadRes *create_default_case_read_res(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    nfs__read_res__init(readres);

    readres->nfs_status = create_nfs_stat(non_nfs_ok_status);
    readres->body_case = NFS__READ_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readres->default_case = empty;

    return readres;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates an ReadDirRes message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the ReadDirRes, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__ReadDirRes *create_default_case_read_dir_res(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    nfs__read_dir_res__init(readdirres);

    readdirres->nfs_status = create_nfs_stat(non_nfs_ok_status);
    readdirres->body_case = NFS__READ_DIR_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readdirres->default_case = empty;

    return readdirres;
}

/*
 * Takes a Nfs__Stat and if it's not NFS__STAT__NFS_OK, creates an StatFsRes message
 * with default case and that status.
 *
 * If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
 *
 * The user of this fuction takes the responsibility to free the StatFsRes, NfsStat,
 * and Empty allocated in this function.
 */
Nfs__StatFsRes *create_default_case_stat_fs_res(Nfs__Stat non_nfs_ok_status) {
    if (non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__StatFsRes *statfsres = malloc(sizeof(Nfs__StatFsRes));
    nfs__stat_fs_res__init(statfsres);

    statfsres->nfs_status = create_nfs_stat(non_nfs_ok_status);
    statfsres->body_case = NFS__STAT_FS_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    statfsres->default_case = empty;

    return statfsres;
}