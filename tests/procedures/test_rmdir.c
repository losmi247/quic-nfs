#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_RMDIR (15) tests
*/

TestSuite(nfs_rmdir_test_suite);

Test(nfs_rmdir_test_suite, rmdir_ok, .description = "NFSPROC_RMDIR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the rmdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *rmdir_test_diropres = lookup_file_or_directory_success(&fhandle, "rmdir_test", NFS__FTYPE__NFDIR);

    // lookup the rmdir_test_dir directory that we are going to delete inside this directory - this will create an inode mapping for it in the inode cache
    Nfs__FHandle rmdir_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle rmdir_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(rmdir_test_diropres->diropok->file->nfs_filehandle);
    rmdir_test_fhandle.nfs_filehandle = &rmdir_test_nfs_filehandle_copy;

    Nfs__DirOpRes *rmdir_test_dir_diropres = lookup_file_or_directory_success(&rmdir_test_fhandle, "rmdir_test_dir", NFS__FTYPE__NFDIR);

    // delete the directory rmdir_test_dir inside this /nfs_share/rmdir_test directory
    Nfs__NfsStat *nfsstat = remove_directory_success(&rmdir_test_fhandle, "rmdir_test_dir");
    nfs__dir_op_res__free_unpacked(rmdir_test_diropres, NULL);

    // read all entries in this directory rmdir_test to ensure rmdir_test_dir was deleted
    int expected_number_of_entries = 2;
    char *expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(&rmdir_test_fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    // try to read from the deleted directory to ensure its inode mapping was deleted from the inode cache
    read_from_directory_fail(rmdir_test_dir_diropres->diropok->file, 0, 30, NFS__STAT__NFSERR_NOENT);

    nfs__dir_op_res__free_unpacked(rmdir_test_dir_diropres, NULL);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

Test(nfs_rmdir_test_suite, rmdir_no_such_containing_directory, .description = "NFSPROC_RMDIR no such containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to remove a directory 'rmdir_test_dir' inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    remove_directory_fail(&fhandle, "rmdir_test_dir", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_rmdir_test_suite, rmdir_directory_in_a_non_directory, .description = "NFSPROC_RMDIR rmdir a directory in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to remove a directory 'rmdir_test_dir' inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    remove_directory_fail(&file_fhandle, "rmdir_test_dir", NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_rmdir_test_suite, rmdir_no_such_directory_to_be_deleted, .description = "NFSPROC_RMDIR no such directory to be deleted") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to remove a nonexistent directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_directory_fail(&fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_NOENT);
}

Test(nfs_rmdir_test_suite, rmdir_not_a_directory, .description = "NFSPROC_RMDIR non-directory specified for a directory operation") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to rmdir the test_file.txt from the mounted directory using RMDIR
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_directory_fail(&fhandle, "test_file.txt", NFS__STAT__NFSERR_NOTDIR);
}