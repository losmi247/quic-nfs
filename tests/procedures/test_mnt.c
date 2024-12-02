#include "tests/test_common.h"

/*
* Mount RPC program tests.
*/

TestSuite(mount_test_suite);

// MOUNTPROC_NULL (0)
Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

// MOUNTPROC_MNT (1)
Test(mount_test_suite, mnt_ok, .description = "MOUNTPROC_MNT ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");
    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_no_such_directory, .description = "MOUNTPROC_MNT no such directory") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/non_existent_directory";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);

    if(status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, MOUNT__STAT__MNTERR_NOENT);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_not_a_directory, .description = "MOUNTPROC_MNT not a directory") {
    // try to mount a file /nfs_share/test_file.txt
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/nfs_share/test_file.txt";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);

    if(status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, MOUNT__STAT__NFSERR_NOTDIR);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_directory_not_exported, .description = "MOUNTPROC_MNT directory not exported") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/existent_but_non_exported_directory";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);

    if(status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, MOUNT__STAT__MNTERR_NOTEXP);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}