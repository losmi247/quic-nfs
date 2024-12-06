#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_MKDIR (14) tests
*/

TestSuite(nfs_mkdir_test_suite);

Test(nfs_mkdir_test_suite, mkdir_ok, .description = "NFSPROC_MKDIR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // create a directory mkdir_test_dir inside this /nfs_share/create_test directory
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(mkdir_test_dir_diropres, NULL);
    mkdir_test_dir_fhandle.nfs_filehandle = &mkdir_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    Nfs__DirOpRes *diropres = create_directory_success(&mkdir_test_dir_fhandle, "mkdir_test_dir", 0440, 0, 0, &atime, &mtime, NFS__FTYPE__NFDIR); // root-owned file (uid=gid=0)

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_mkdir_test_suite, mkdir_no_such_directory, .description = "NFSPROC_MKDIR no such directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to create a directory mkdir_test_dir inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(&fhandle, "mkdir_test_dir", 0440, 0, 0, &atime, &mtime, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_mkdir_test_suite, mkdir_directory_in_a_non_directory, .description = "NFSPROC_MKDIR create directory in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a directory mkdir_test_dir inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(&file_fhandle, "mkdir_test_dir", 0440, 0, 0, &atime, &mtime, NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_mkdir_test_suite, mkdir_file_name_too_long, .description = "NFSPROC_MKDIR file name too long") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the mkdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // try to create a directory with a too long filename inside this /nfs_share/mkdir_test directory
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(mkdir_test_dir_diropres, NULL);
    mkdir_test_dir_fhandle.nfs_filehandle = &mkdir_test_dir_nfs_filehandle_copy;

    char *filename = malloc(sizeof(char) * (NFS_MAXNAMLEN + 20));
    memset(filename, 'a', NFS_MAXNAMLEN + 19);
    filename[NFS_MAXNAMLEN + 19] = '\0'; // null terminate the name
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(&mkdir_test_dir_fhandle, filename, 0440, 0, 0, &atime, &mtime, NFS__STAT__NFSERR_NAMETOOLONG);
}

Test(nfs_mkdir_test_suite, mkdir_already_existing_directory, .description = "NFSPROC_MKDIR create already existing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the mkdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // try to create a file /nfs_share/mkdir_test/existing_directory that already exists
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(mkdir_test_dir_diropres, NULL);
    mkdir_test_dir_fhandle.nfs_filehandle = &mkdir_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(&mkdir_test_dir_fhandle, "existing_directory", 0440, 0, 0, &atime, &mtime, NFS__STAT__NFSERR_EXIST);
}