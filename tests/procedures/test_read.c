#include "tests/test_common.h"

/*
* NFSPROC_READ (6) tests
*/

TestSuite(nfs_read_test_suite);

Test(nfs_read_test_suite, read_ok, .description = "NFSPROC_READ ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

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

    // validate DirOpRes
    cr_assert_eq(diropres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DIROPOK);
    cr_assert_not_null(diropres->diropok);

    cr_assert_not_null(diropres->diropok->file);
    cr_assert_not_null(diropres->diropok->file->nfs_filehandle);     // can't validate NFS filehandle contents as a client
    Nfs__FAttr *fattr = diropres->diropok->attributes;
    validate_fattr(fattr, NFS__FTYPE__NFREG); // can't validate any other attributes

    // read from test_file.txt
    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = diropres->diropok->file;
    readargs.count = 10;
    readargs.offset = 2;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    status = nfs_procedure_6_read_from_file(readargs, readres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);
        nfs__dir_op_res__free_unpacked(diropres, NULL);

        cr_fail("NFSPROC_READ failed - status %d\n", status);
    }

    // validate ReadRes
    cr_assert_eq(readres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_READRESBODY);
    cr_assert_not_null(readres->readresbody);

    // validate attributes
    cr_assert_not_null(readres->readresbody->attributes);
    Nfs__FAttr *read_fattr = readres->readresbody->attributes;
    validate_fattr(read_fattr, NFS__FTYPE__NFREG);
    check_equal_fattr(fattr, read_fattr);
    // this is a famous problem - is atime going to be flushed to disk by the kernel after every single read - no, only after a day, nothing I can do about it
    cr_assert(get_time(fattr->atime) <= get_time(read_fattr->atime));   // file was accessed
    cr_assert(get_time(fattr->mtime) == get_time(read_fattr->mtime));   // file was not modified
    cr_assert(get_time(fattr->ctime) == get_time(read_fattr->ctime));

    // validate read content
    cr_assert_not_null(readres->readresbody->nfsdata.data);
    ProtobufCBinaryData read_content = readres->readresbody->nfsdata;
    cr_assert_eq(read_content.len, 10);

    char *read_content_as_string = malloc(sizeof(char) * (read_content.len + 1));
    memcpy(read_content_as_string, read_content.data, read_content.len);
    read_content_as_string[read_content.len] = 0;   // null terminate the string

    char *test_file_content = "test_content";

    cr_assert_str_eq(read_content_as_string, test_file_content + 2);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    nfs__read_res__free_unpacked(readres, NULL);
}

Test(nfs_read_test_suite, read_no_such_file, .description = "NFSPROC_READ no such file") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to read from a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = 12345678912345;
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

Test(nfs_read_test_suite, read_is_directory, .description = "NFSPROC_READ directory specified for a non directory operation") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

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