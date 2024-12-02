#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_CREATE (9) tests
*/

TestSuite(nfs_create_test_suite);

Test(nfs_create_test_suite, create_ok, .description = "NFSPROC_CREATE ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory(&fhandle, "create_test", NFS__FTYPE__NFDIR);

    // create a file create_test.txt inside this /nfs_share/create_test directory
    Nfs__FHandle create_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle create_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(create_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(create_test_dir_diropres, NULL);
    create_test_dir_fhandle.nfs_filehandle = &create_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    Nfs__DirOpRes *diropres = create_file_success(&create_test_dir_fhandle, "create_test_file.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__FTYPE__NFREG); // root-owned file (uid=gid=0)

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_create_test_suite, create_no_such_directory, .description = "NFSPROC_CREATE no such directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to create a file 'create_test.txt' inside a nonexistent directory
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
    create_file_fail(&fhandle, "create_test.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_create_test_suite, create_file_in_a_non_directory, .description = "NFSPROC_CREATE create file in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a file "create_test.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_file_fail(&file_fhandle, "create_test.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_create_test_suite, create_file_name_too_long, .description = "NFSPROC_CREATE file name too long") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory(&fhandle, "create_test", NFS__FTYPE__NFDIR);

    // try to create a file with a too long filename inside this /nfs_share/create_test directory
    Nfs__FHandle create_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle create_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(create_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(create_test_dir_diropres, NULL);
    create_test_dir_fhandle.nfs_filehandle = &create_test_dir_nfs_filehandle_copy;

    char *filename = malloc(sizeof(char) * (NFS_MAXNAMLEN + 20));
    memset(filename, 'a', NFS_MAXNAMLEN + 19);
    filename[NFS_MAXNAMLEN + 19] = '\0'; // null terminate the name
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_file_fail(&create_test_dir_fhandle, filename, 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NAMETOOLONG);
}

Test(nfs_create_test_suite, create_already_existing_file, .description = "NFSPROC_CREATE create already existing file") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory(&fhandle, "create_test", NFS__FTYPE__NFDIR);

    // try to create a file /nfs_share/create_test/existing_file.txt that already exists
    Nfs__FHandle create_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle create_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(create_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(create_test_dir_diropres, NULL);
    create_test_dir_fhandle.nfs_filehandle = &create_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_file_fail(&create_test_dir_fhandle, "existing_file.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_EXIST);
}