#include "common_validation.h"

/*
* Gives a string form of the given file type.
*
* The user of this function takes the responsibility to free the received string.
*/
char *file_type_to_string(Nfs__FType ftype) {
    switch(ftype) {
        case NFS__FTYPE__NFREG:
            return strdup("NFREG");
        case NFS__FTYPE__NFDIR:
            return strdup("NFDIR");
        case NFS__FTYPE__NFLNK:
            return strdup("NFLNK");
        case NFS__FTYPE__NFCHR:
            return strdup("NFCHR");
        case NFS__FTYPE__NFBLK:
            return strdup("NFBLK");
        case NFS__FTYPE__NFNON:
            return strdup("NFNON");
        default:
            return strdup("Unknown");
    }
}

/*
* Validates FAttr fields that can be checked for correctnes from the client.
*/
void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype) {
    cr_assert_not_null(fattr);

    cr_assert_not_null(fattr->nfs_ftype);

    char *expected_file_type = file_type_to_string(ftype), *found_file_type = file_type_to_string(fattr->nfs_ftype->ftype);
    cr_assert_eq(fattr->nfs_ftype->ftype, ftype, "Expected file type %s but found file type %s", expected_file_type, found_file_type);
    free(expected_file_type);
    free(found_file_type);

    cr_assert_not_null(fattr->atime);
    cr_assert_not_null(fattr->mtime);
    cr_assert_not_null(fattr->ctime);
}

/*
* Checks two FAttr structures obtained from the same file for equality, 
* excluding atime, mtime, ctime fields.
*/
void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2) {
    cr_assert_not_null(fattr1);
    cr_assert_not_null(fattr2);
    cr_assert_not_null(fattr1->nfs_ftype);
    cr_assert_not_null(fattr2->nfs_ftype);

    cr_assert_eq(fattr1->nfs_ftype->ftype, fattr2->nfs_ftype->ftype);
    cr_assert_eq(fattr1->mode, fattr2->mode);

    cr_assert_eq(fattr1->nlink, fattr2->nlink);
    cr_assert_eq(fattr1->uid, fattr2->uid);
    cr_assert_eq(fattr1->gid, fattr2->gid);

    cr_assert_eq(fattr1->size, fattr2->size);
    cr_assert_eq(fattr1->blocksize, fattr2->blocksize);
    cr_assert_eq(fattr1->rdev, fattr2->rdev);
    cr_assert_eq(fattr1->blocks, fattr2->blocks);
    cr_assert_eq(fattr1->fsid, fattr2->fsid);
    cr_assert_eq(fattr1->fileid, fattr2->fileid);
}

/*
* Given the SAttr used to update file attributes in SETATTR procedure or initialize file attributes
* or initialize file attributes in CREATE procedure, checks that the updated file attributes have
* proper values after the update.
*/
void check_fattr_update(Nfs__FAttr *updated_fattr, Nfs__SAttr *sattr) {
    cr_assert_not_null(updated_fattr);
    cr_assert_not_null(updated_fattr->atime);
    cr_assert_not_null(updated_fattr->mtime);
    cr_assert_not_null(sattr);
    cr_assert_not_null(sattr->atime);
    cr_assert_not_null(sattr->mtime);

    if(sattr->mode != -1) {
        cr_assert_eq(((updated_fattr->mode) & ((1<<12)-1)), sattr->mode); // only check first 12 bits - (sticky,setuid,setgid),owner,group,others, before that is device/file type
    }

    if(sattr->uid != -1) {
        cr_assert_eq(updated_fattr->uid, sattr->uid);
    }
    if(sattr->gid != -1) {
        cr_assert_eq(updated_fattr->gid, sattr->gid);
    }

    if(sattr->size != -1) {
        cr_assert_eq(updated_fattr->size, sattr->size);
    }

    Nfs__TimeVal *atime = sattr->atime, *mtime = sattr->mtime;
    if(atime->seconds != -1 && atime->useconds != -1 && mtime->seconds != -1 && mtime->useconds != -1) {
        cr_assert_eq(updated_fattr->atime->seconds, atime->seconds);
        cr_assert_eq(updated_fattr->atime->useconds, atime->useconds);
        cr_assert_eq(updated_fattr->mtime->seconds, mtime->seconds);
        cr_assert_eq(updated_fattr->mtime->useconds, mtime->useconds);
    }
}