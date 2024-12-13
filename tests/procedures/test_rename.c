#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_RENAME (11) tests
*/

TestSuite(nfs_rename_test_suite);

Test(nfs_rename_test_suite, rename_ok, .description = "NFSPROC_RENAME ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the rename_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *rename_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "rename_test", NFS__FTYPE__NFDIR);

    // lookup the dir1 and dir2 directories inside the rename_test directory
    Nfs__FHandle rename_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle rename_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(rename_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(rename_test_dir_diropres, NULL);
    rename_test_dir_fhandle.nfs_filehandle = &rename_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *dir1_diropres = lookup_file_or_directory_success(&rename_test_dir_fhandle, "dir1", NFS__FTYPE__NFDIR);
    Nfs__DirOpRes *dir2_diropres = lookup_file_or_directory_success(&rename_test_dir_fhandle, "dir2", NFS__FTYPE__NFDIR);

    // nfs__dir_op_res__free_unpacked(dir_diropres, NULL); !!!!!

    // lookup the rename_test_file.txt that we are going to rename inside dir1 directory - this will create an inode mapping for it in the inode cache
    Nfs__FHandle dir1_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir1_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir1_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir1_diropres, NULL);
    dir1_fhandle.nfs_filehandle = &dir1_nfs_filehandle_copy;

    Nfs__DirOpRes *rename_test_file_diropres = lookup_file_or_directory_success(&dir1_fhandle, "rename_test_file.txt", NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(rename_test_file_diropres, NULL);

    // rename the file rename_test_file.txt inside this /nfs_share/rename_test/dir1 directory to directory /nfs_share/rename_test/dir2/renamed_file.txt
    Nfs__FHandle dir2_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir2_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir2_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir2_diropres, NULL);
    dir2_fhandle.nfs_filehandle = &dir2_nfs_filehandle_copy;

    Nfs__NfsStat *nfsstat = rename_file_success(&dir1_fhandle, "rename_test_file.txt", &dir2_fhandle, "renamed_file.txt");

    // read the directory dir1 to ensure the file was moved from there
    int dir1_expected_number_of_entries = 2;
    char *dir1_expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *dir1_readdirres = read_from_directory_success(&dir1_fhandle, 0, 1000, dir1_expected_number_of_entries, dir1_expected_filenames);
    nfs__read_dir_res__free_unpacked(dir1_readdirres, NULL);

    // read the directory dir2 to ensure the file was moved to there
    int dir2_expected_number_of_entries = 3;
    char *dir2_expected_filenames[3] = {"..", ".", "renamed_file.txt"};

    Nfs__ReadDirRes *dir2_readdirres = read_from_directory_success(&dir2_fhandle, 0, 1000, dir2_expected_number_of_entries, dir2_expected_filenames);
    nfs__read_dir_res__free_unpacked(dir2_readdirres, NULL);
    
    // try to lookup the renamed file at the old location to ensure its inode mapping was updated in the inode cache
    lookup_file_or_directory_fail(&dir1_fhandle, "rename_test_file.txt", NFS__STAT__NFSERR_NOENT);
    // lookup the renamed file at the new location to ensure its absolute path was updated in the inode cache
    Nfs__DirOpRes *renamed_file_diropres = lookup_file_or_directory_success(&dir2_fhandle, "renamed_file.txt", NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(renamed_file_diropres, NULL);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

Test(nfs_rename_test_suite, rename_no_such_from_directory, .description = "NFSPROC_RENAME no such 'from' directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to rename a file 'rename_test_file.txt' inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    rename_file_fail(&fhandle, "remove_file_test.txt", &fhandle, "renamed_file.txt", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_rename_test_suite, move_file_from_a_non_directory, .description = "NFSPROC_RENAME move file from a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to rename a file "rename_test_file.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    rename_file_fail(&file_fhandle, "rename_test_file.txt", &fhandle, "renamed_file.txt" ,NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_rename_test_suite, rename_no_such_file, .description = "NFSPROC_REMOVE no such file") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to rename a nonexistent file
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    rename_file_fail(&fhandle, NONEXISTENT_FILENAME, &fhandle, "renamed_file.txt",NFS__STAT__NFSERR_NOENT);
}

Test(nfs_rename_test_suite, rename_no_such_to_directory, .description = "NFSPROC_RENAME no such 'to' directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to move the file 'test_file.txt' in the mounted to directory to a nonexistent directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    NfsFh__NfsFileHandle to_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    to_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    to_nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle to_fhandle = NFS__FHANDLE__INIT;
    to_fhandle.nfs_filehandle = &to_nfs_filehandle;

    rename_file_fail(&fhandle, "test_file.txt", &to_fhandle, "renamed_file.txt", NFS__STAT__NFSERR_NOENT);
}

Test(nfs_rename_test_suite, move_file_to_a_non_directory, .description = "NFSPROC_RENAME move file to a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to move the file "test_file.txt" inside itself test_file.txt (i.e. 'to' is a n)
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    rename_file_fail(&fhandle, "test_file.txt", &file_fhandle, "renamed_file.txt" ,NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_rename_test_suite, make_directory_a_subdirectory_of_itself, .description = "NFSPROC_REMOVE make directory a subdirectory of itself") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the rename_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *rename_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "rename_test", NFS__FTYPE__NFDIR);

    // lookup the dir3 directory inside the rename_test directory
    Nfs__FHandle rename_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle rename_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(rename_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(rename_test_dir_diropres, NULL);
    rename_test_dir_fhandle.nfs_filehandle = &rename_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *dir3_diropres = lookup_file_or_directory_success(&rename_test_dir_fhandle, "dir3", NFS__FTYPE__NFDIR);

    // try to move the /nfs_share/rename_test/dir3 directory to /nfs_share/rename_test/dir3/new_dir to make it a subdirectory of itself
    Nfs__FHandle dir3_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir3_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir3_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir3_diropres, NULL);
    dir3_fhandle.nfs_filehandle = &dir3_nfs_filehandle_copy;

    rename_file_fail(&rename_test_dir_fhandle, "dir3", &dir3_fhandle, "newdir", NFS__STAT__NFSERR_EXIST);
}

Test(nfs_rename_test_suite, overwrite_non_empty_directory, .description = "NFSPROC_REMOVE overwrite a nonempty directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the rename_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *rename_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "rename_test", NFS__FTYPE__NFDIR);

    // try to move the /nfs_share/rename_test/dir3 directory to /nfs_share/rename_test/dir4 (overwrite dir4 which is a nonempty directory)
    Nfs__FHandle rename_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle rename_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(rename_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(rename_test_dir_diropres, NULL);
    rename_test_dir_fhandle.nfs_filehandle = &rename_test_dir_nfs_filehandle_copy;
}