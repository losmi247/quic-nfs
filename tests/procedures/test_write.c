#include "tests/test_common.h"

#include <stdio.h>

/*
* NFSPROC_WRITE (8) tests
*/

TestSuite(nfs_write_test_suite);

Test(nfs_write_test_suite, write_ok, .description = "NFSPROC_WRITE ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the write_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *write_test_dir_diropres = lookup_file_or_directory(&fhandle, "write_test", NFS__FTYPE__NFDIR);

    // lookup the write_test_file.txt inside this /nfs_share/write_test directory
    Nfs__FHandle write_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle write_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(write_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(write_test_dir_diropres, NULL);
    write_test_dir_fhandle.nfs_filehandle = &write_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&write_test_dir_fhandle, "write_test_file.txt", NFS__FTYPE__NFREG);

    // write to this write_test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__AttrStat *attrstat = write_to_file(&file_fhandle, 6, 5, "done", NFS__FTYPE__NFREG);

    // read from write_test_file.txt
    Nfs__ReadRes *readres = read_from_file(&file_fhandle, 0, 20, attrstat->attributes, NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    nfs__attr_stat__free_unpacked(attrstat, NULL);

    // validate read content
    ProtobufCBinaryData read_content = readres->readresbody->nfsdata;
    char *read_content_as_string = malloc(sizeof(char) * (read_content.len + 1));
    memcpy(read_content_as_string, read_content.data, read_content.len);
    read_content_as_string[read_content.len] = 0;   // null terminate the string

    char *new_test_file_content = "write_done";
    cr_assert_str_eq(read_content_as_string, new_test_file_content, "Expected %s, but found %s", new_test_file_content, read_content_as_string);

    free(read_content_as_string);

    nfs__read_res__free_unpacked(readres, NULL);
}

Test(nfs_write_test_suite, write_no_such_file, .description = "NFSPROC_WRITE no such file") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to write to a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    file_nfs_filehandle.timestamp = 0;

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = &file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = 2;
    writeargs.totalcount = 0;  // unused
    writeargs.nfsdata.data = "write";
    writeargs.nfsdata.len = 5;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fail("NFSPROC_WRITE failed - status %d\n", status);
    }

    // validate AttrStat
    cr_assert_eq(attrstat->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_write_test_suite, write_is_directory, .description = "NFSPROC_WRITE directory specified for a non-directory operation") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to write to the mounted directory
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = &file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = 2;
    writeargs.totalcount = 0;  // unused
    writeargs.nfsdata.data = "write";
    writeargs.nfsdata.len = 5;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attrstat);

        cr_fail("NFSPROC_WRITE failed - status %d\n", status);
    }

    // validate AttrStat
    cr_assert_eq(attrstat->status, NFS__STAT__NFSERR_ISDIR);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_write_test_suite, write_too_much_data, .description = "NFSPROC_WRITE more than NFS_MAXDATA bytes in nfsdata") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the write_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *write_test_dir_diropres = lookup_file_or_directory(&fhandle, "write_test", NFS__FTYPE__NFDIR);

    // lookup the write_test_file.txt inside this /nfs_share/write_test directory
    Nfs__FHandle write_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle write_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(write_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(write_test_dir_diropres, NULL);
    write_test_dir_fhandle.nfs_filehandle = &write_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&write_test_dir_fhandle, "write_test_file.txt", NFS__FTYPE__NFREG);

    // try to write 10000 bytes in a single RPC to this write_test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = &file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = 6;
    writeargs.totalcount = 0;  // unused

    uint8_t *content = malloc(sizeof(uint8_t) * (NFS_MAXDATA + 20));
    memset(content, 'a', NFS_MAXDATA + 20);
    writeargs.nfsdata.data = content;
    writeargs.nfsdata.len = NFS_MAXDATA + 20;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fail("NFSPROC_WRITE failed - status %d\n", status);
    }

    // validate AttrStat
    cr_assert_eq(attrstat->status, NFS__STAT__NFSERR_FBIG);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    free(content);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}