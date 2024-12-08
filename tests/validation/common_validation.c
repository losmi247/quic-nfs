#include "common_validation.h"

/*
* Validates FAttr fields that can be checked for correctnes from the client.
*/
void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype) {
    cr_assert_eq(fattr->type, ftype);

    cr_assert_not_null(fattr->atime);
    cr_assert_not_null(fattr->mtime);
    cr_assert_not_null(fattr->ctime);
}

/*
* Checks two FAttr structures obtained from the same file for equality, 
* excluding atime, mtime, ctime fields.
*/
void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2) {
    cr_assert_eq(fattr1->type, fattr2->type);
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