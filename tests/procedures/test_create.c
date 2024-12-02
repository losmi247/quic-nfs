#include "tests/test_common.h"

#include <stdio.h>

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
    Nfs__DirOpRes *diropres = create_file(&create_test_dir_fhandle, "create_test_file.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__FTYPE__NFREG); // root-owned file (uid=gid=0)

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}
