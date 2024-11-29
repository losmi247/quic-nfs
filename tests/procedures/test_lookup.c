#include "tests/test_common.h"

#include <time.h>

/*
* NFSPROC_LOOKUP (4) tests
*/

TestSuite(nfs_lookup_test_suite);

Test(nfs_lookup_test_suite, lookup_ok, .description = "NFSPROC_LOOKUP ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_lookup_test_suite, lookup_no_such_directory, .description = "NFSPROC_LOOKUP no such directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup a test_file.txt inside a different nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = "test_file.txt";

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = &fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(diropargs, diropres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(diropres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    cr_assert_eq(diropres->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_lookup_test_suite, lookup_no_such_file_or_directory, .description = "NFSPROC_LOOKUP no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup a nonexistent file inside a this mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = "nonexistent_file.txt";

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = &fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(diropargs, diropres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(diropres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    cr_assert_eq(diropres->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
}