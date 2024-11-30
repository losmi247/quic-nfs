#include "common_validation.h"

/*
* Validates FAttr fields that can be check for correctnes from the client.
*/
void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype) {
    cr_assert_neq(fattr, NULL);

    cr_assert_eq(fattr->type, ftype);
    cr_assert_not_null(fattr->atime);
    cr_assert_not_null(fattr->mtime);
    cr_assert_not_null(fattr->ctime);

    // other fattr fields are difficult to validate
}

/*
* Checks two FAttr structures of same file for equality, excluding atime, mtime, ctime fields.
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