#include "tests/test_common.h"

/*
* NFSPROC_SETATTR (2) tests
*/

TestSuite(nfs_setattr_test_suite);

Test(nfs_setattr_test_suite, setattr_ok, .description = "NFSPROC_SETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // now update attributes of /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    
    Nfs__AttrStat *attrstat = set_attributes_success(&fhandle, 0, 0, 0, -1, &atime, &mtime, NFS__FTYPE__NFDIR);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_setattr_test_suite, setattr_no_such_file_or_directory, .description = "NFSPROC_SETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // pick a nonexistent inode number in this mounted directory and try to update its attributes
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;

    set_attributes_fail(&fhandle, 0, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}