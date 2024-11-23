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
        // it's hard to validate the nfs filehandle at client, so we don't do it
        //cr_assert_str_eq((unsigned char *) fhstatus->directory->handle.data, nfs_share_inode_number);
        cr_assert_eq(fhstatus->status, 0);
        cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);
        cr_assert_neq((unsigned char *) fhstatus->directory, NULL);

        return fhstatus;
    } else {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);

        return NULL; // do we need to do this after cr_fail?
    }
}

/*
*
*/
void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype) {
    cr_assert_neq(fattr, NULL);

    cr_assert_eq(fattr->type, ftype);
    cr_assert_neq(fattr->atime, NULL);
    cr_assert_neq(fattr->mtime, NULL);
    cr_assert_neq(fattr->ctime, NULL);

    // other fields are difficult to validate
}