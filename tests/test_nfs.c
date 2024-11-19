#include "criterion/criterion.h"
#include <criterion/new/assert.h>

#include <stdlib.h>
#include <stdio.h>

#include "src/nfs/clients/mount_client.h"
#include "src/nfs/clients/nfs_client.h"

TestSuite(mount_test_suite);

Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

Test(mount_test_suite, mnt, .description = "MOUNTPROC_MNT") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if (status == 0) {
        // it's hard to validate the nfs filehandle at client, so we don't do it
        //cr_assert_str_eq((unsigned char *) fhstatus->directory->handle.data, nfs_share_inode_number);
        cr_assert_eq(fhstatus->status, 0);
        cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);
        cr_assert_neq((unsigned char *) fhstatus->directory, NULL);

        mount__fh_status__free_unpacked(fhstatus, NULL);
    } else {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }
}

TestSuite(nfs_test_suite);

Test(nfs_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

Test(nfs_test_suite, getattr, .description = "NFSPROC_GETATTR") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if(status != 0) {
        free(fhstatus);

        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    // it's hard to validate the nfs filehandle at client, so we don't do it
    //cr_assert_str_eq((unsigned char *) fhstatus->directory->handle.data, nfs_share_inode_number);
    cr_assert_eq(fhstatus->status, 0);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);
    cr_assert_neq((unsigned char *) fhstatus->directory, NULL);

    // now get file attributes
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.handle = fhstatus->directory->handle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    cr_assert_neq(attr_stat->attributes, NULL);

    Nfs__FAttr *fattr = attr_stat->attributes;
    cr_assert_eq(fattr->type, NFS__FTYPE__NFDIR);
    cr_assert_eq(fattr->nlink, 2); // when a directory is created, number of links to it is 2
    cr_assert_neq(fattr->atime, NULL);
    cr_assert_neq(fattr->mtime, NULL);
    cr_assert_neq(fattr->ctime, NULL);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free(attr_stat);
}
