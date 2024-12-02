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

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // read from this test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__ReadRes *readres = read_from_file(&file_fhandle, 2, 10, diropres->diropok->attributes, NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    // validate read content
    ProtobufCBinaryData read_content = readres->readresbody->nfsdata;
    char *read_content_as_string = malloc(sizeof(char) * (read_content.len + 1));
    memcpy(read_content_as_string, read_content.data, read_content.len);
    read_content_as_string[read_content.len] = 0;   // null terminate the string

    char *test_file_content = "test_content";
    cr_assert_str_eq(read_content_as_string, test_file_content + 2);

    nfs__read_res__free_unpacked(readres, NULL);
}

Test(nfs_read_test_suite, read_no_such_file, .description = "NFSPROC_READ no such file") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to read from a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    file_nfs_filehandle.timestamp = 0;

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = &file_fhandle;
    readargs.count = 10;
    readargs.offset = 2;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    int status = nfs_procedure_6_read_from_file(readargs, readres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        cr_fail("NFSPROC_READ failed - status %d\n", status);
    }

    // validate ReadRes
    cr_assert_eq(readres->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_res__free_unpacked(readres, NULL);
}

Test(nfs_read_test_suite, read_is_directory, .description = "NFSPROC_READ directory specified for a non-directory operation") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to read from the mounted directory
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = &file_fhandle;
    readargs.count = 10;
    readargs.offset = 2;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    int status = nfs_procedure_6_read_from_file(readargs, readres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        cr_fail("NFSPROC_READ failed - status %d\n", status);
    }

    // validate ReadRes
    cr_assert_eq(readres->status, NFS__STAT__NFSERR_ISDIR);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_res__free_unpacked(readres, NULL);
}