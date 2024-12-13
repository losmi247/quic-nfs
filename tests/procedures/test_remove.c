#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_REMOVE (10) tests
*/

TestSuite(nfs_remove_test_suite);

Test(nfs_remove_test_suite, remove_ok, .description = "NFSPROC_REMOVE ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the remove_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *remove_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "remove_test", NFS__FTYPE__NFDIR);

    // lookup the remove_test_file.txt that we are going to delete inside this directory - this will create an inode mapping for it in the inode cache
    Nfs__FHandle remove_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle remove_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(remove_test_dir_diropres->diropok->file->nfs_filehandle);
    remove_test_dir_fhandle.nfs_filehandle = &remove_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *remove_test_file_diropres = lookup_file_or_directory_success(&remove_test_dir_fhandle, "remove_test_file.txt", NFS__FTYPE__NFREG);

    // remove the file remove_test_file.txt inside this /nfs_share/remove_test directory
    Nfs__NfsStat *nfsstat = remove_file_success(&remove_test_dir_fhandle, "remove_test_file.txt");
    nfs__dir_op_res__free_unpacked(remove_test_dir_diropres, NULL);

    // read all entries in this directory remove_test to ensure remove_test_file.txt was deleted
    int expected_number_of_entries = 2;
    char *expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(&remove_test_dir_fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    // try to read from the deleted file to ensure its inode mapping was deleted from the inode cache
    read_from_file_fail(remove_test_file_diropres->diropok->file, 0, 10, NFS__STAT__NFSERR_NOENT);

    nfs__dir_op_res__free_unpacked(remove_test_file_diropres, NULL);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

Test(nfs_remove_test_suite, remove_no_such_directory, .description = "NFSPROC_REMOVE no such directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to remove a file 'remove_test_file.txt' inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    remove_file_fail(&fhandle, "remove_file_test.txt", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_remove_test_suite, remove_file_in_a_non_directory, .description = "NFSPROC_REMOVE remove file in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to remove a file "remove_test_file.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    remove_file_fail(&file_fhandle, "remove_test_file.txt", NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_remove_test_suite, remove_no_such_file, .description = "NFSPROC_REMOVE no such file") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to remove a nonexistent file
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_file_fail(&fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_NOENT);
}

Test(nfs_remove_test_suite, remove_is_directory, .description = "NFSPROC_REMOVE directory specified for a non-directory operation") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to remove the remove_test directory from the mounted directory using REMOVE
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_file_fail(&fhandle, "remove_test", NFS__STAT__NFSERR_ISDIR);
}