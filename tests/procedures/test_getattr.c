#include "tests/test_common.h"

/*
* NFSPROC_GETATTR (1) tests
*/

TestSuite(nfs_getattr_test_suite);

Test(nfs_getattr_test_suite, getattr_ok, .description = "NFSPROC_GETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // now get file attributes for the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__AttrStat *attrstat = get_attributes_success(fhandle, NFS__FTYPE__NFDIR);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_getattr_test_suite, getattr_no_such_file_or_directory, .description = "NFSPROC_GETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // pick a nonexistent inode number in this mounted directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    get_attributes_fail(fhandle, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

