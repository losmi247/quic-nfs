#include "tests/test_common.h"

/*
* NFSPROC_READ (6) tests
*/

TestSuite(nfs_read_test_suite);

Test(nfs_read_test_suite, read_ok, .description = "NFSPROC_READ ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // read from this test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    char *expected_test_file_content = "test_content";
    Nfs__ReadRes *readres = read_from_file_success(&file_fhandle, 2, 10, diropres->diropok->attributes, expected_test_file_content + 2);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    nfs__read_res__free_unpacked(readres, NULL);
}

Test(nfs_read_test_suite, read_no_such_file, .description = "NFSPROC_READ no such file") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to read from a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    file_nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

    read_from_file_fail(&file_fhandle, 2, 10, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_read_test_suite, read_is_directory, .description = "NFSPROC_READ directory specified for a non-directory operation") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to read from the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    read_from_file_fail(&fhandle, 2, 10, NFS__STAT__NFSERR_ISDIR);
}