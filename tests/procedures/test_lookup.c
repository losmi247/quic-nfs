#include "tests/test_common.h"

/*
* NFSPROC_LOOKUP (4) tests
*/

TestSuite(nfs_lookup_test_suite);

Test(nfs_lookup_test_suite, lookup_ok, .description = "NFSPROC_LOOKUP ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(&dir_fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_lookup_test_suite, lookup_inside_non_existent_directory, .description = "NFSPROC_LOOKUP inside non-existent directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup a test_file.txt inside a different nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    lookup_file_or_directory_fail(&fhandle, "test_file.txt", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_lookup_test_suite, lookup_no_such_file_or_directory, .description = "NFSPROC_LOOKUP no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup a nonexistent file inside this mounted directory
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    lookup_file_or_directory_fail(&dir_fhandle, NONEXISTENT_FILE_NAME, NFS__STAT__NFSERR_NOENT);
}

Test(nfs_lookup_test_suite, lookup_a_non_directory, .description = "NFSPROC_LOOKUP lookup a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to lookup a file name "a.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    lookup_file_or_directory_fail(&file_fhandle, "a.txt", NFS__STAT__NFSERR_NOTDIR);
}