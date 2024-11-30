#include "tests/test_common.h"

/*
* NFSPROC_GETATTR (1) tests
*/

TestSuite(nfs_getattr_test_suite);

// NFSPROC_GETATTR (1)
Test(nfs_getattr_test_suite, getattr_ok, .description = "NFSPROC_GETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // now get file attributes
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    validate_fattr(attr_stat->attributes, NFS__FTYPE__NFDIR);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

Test(nfs_getattr_test_suite, getattr_no_such_file_or_directory, .description = "NFSPROC_GETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // pick a nonexistent inode number in this mounted directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attr_stat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}
