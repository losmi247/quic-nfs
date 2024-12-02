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
    Nfs__AttrStat *attrstat = set_attributes(&fhandle, 0, 0, 0, -1, &atime, &mtime, NFS__FTYPE__NFDIR);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_setattr_test_suite, setattr_no_such_file_or_directory, .description = "NFSPROC_SETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // pick a nonexistent inode number in this mounted directory and try to update its attributes
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = 0;
    sattr.uid = 0;   // make it a root-owned file
    sattr.gid = 0;
    sattr.size = 10;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = &fhandle;
    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_SETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attr_stat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}