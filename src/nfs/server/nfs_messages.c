#include "nfs_messages.h"

/*
* Takes a Nfs__Stat and if it's NFS__STAT__NFS_OK, creates an AttrStat message
* with default case and that status.
*
* If the given Nfs__Stat is NFS__STAT__NFS_OK, NULL is returned.
*
* The user of this fuction takes the responsibility to free the AttrStat and Empty 
* allocated in this function.
*/
Nfs__AttrStat *create_default_case_attr_stat(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    nfs__attr_stat__init(attr_stat);
    attr_stat->status = non_nfs_ok_status;
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
* The user of this fuction takes the responsibility to free the DirOpRes and Empty 
* allocated in this function.
*/
Nfs__DirOpRes *create_default_case_dir_op_res(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    nfs__dir_op_res__init(diropres);
    diropres->status = non_nfs_ok_status;
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
* The user of this fuction takes the responsibility to free the ReadRes and Empty 
* allocated in this function.
*/
Nfs__ReadRes *create_default_case_read_res(Nfs__Stat non_nfs_ok_status) {
    if(non_nfs_ok_status == NFS__STAT__NFS_OK) {
        return NULL;
    }

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    nfs__read_res__init(readres);
    readres->status = non_nfs_ok_status;
    readres->body_case = NFS__READ_RES__BODY_DEFAULT_CASE;

    Google__Protobuf__Empty *empty = malloc(sizeof(Google__Protobuf__Empty));
    google__protobuf__empty__init(empty);
    readres->default_case = empty;

    return readres;
}