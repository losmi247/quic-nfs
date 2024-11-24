#include "test_common.h"

/*
* Make testing easier by first mounting the directory given by absolute path, and return 
* the fhstatus on success.
*
* The user of this function takes on the responsibility to call 'mount_fh_status_free_unpacked()'
* with the obtained fhstatus in case of successful execution.
*/
Mount__FhStatus *mount_directory(char *directory_absolute_path) {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = directory_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if (status == 0) {
        cr_assert_eq(fhstatus->status, 0);
        cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);

        cr_assert_not_null(fhstatus->directory);
        cr_assert_not_null(fhstatus->directory->nfs_filehandle);
        // it's hard to validate the nfs filehandle at client, so we don't do it

        return fhstatus;
    } else {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);

        return NULL;
    }
}

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

uint64_t get_time(Nfs__TimeVal *timeval) {
    uint64_t sec = timeval->seconds;
    uint64_t milisec = timeval->useconds;
    return sec * 1000 + milisec;
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