#include "test_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
* Mount RPC program tests.
*/

TestSuite(mount_test_suite);

// MOUNTPROC_NULL (0)
Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

// MOUNTPROC_MNT (1)
Test(mount_test_suite, mnt_ok, .description = "MOUNTPROC_MNT ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");
    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_directory_not_exported, .description = "MOUNTPROC_MNT directory not exported") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/existent_but_non_exported_directory";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);

    if(status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, 1);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(mount_test_suite, mnt_no_such_directory, .description = "MOUNTPROC_MNT no such directory") {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = "/exported_but_non_existent_directory";

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);

    if(status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, 2);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Nfs RPC program tests.
*/

TestSuite(nfs_test_suite);

// NFSPROC_NULL (0)
Test(nfs_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

// NFSPROC_GETATTR (1)
Test(nfs_test_suite, getattr_ok, .description = "NFSPROC_GETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // now get file attributes
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    validate_fattr(attr_stat->attributes, NFS__FTYPE__NFDIR);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

Test(nfs_test_suite, getattr_no_such_file_or_directory, .description = "NFSPROC_GETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // pick a nonexistent inode number in this mounted directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(fhandle, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attr_stat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

// NFSPROC_SETATTR (2)
Test(nfs_test_suite, setattr_ok, .description = "NFSPROC_SETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // now update attributes of /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = 0;
    sattr.uid = 0;   // make it a root-owned file
    sattr.gid = 0;
    sattr.size = -1; // can't update size on a directory
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = &fhandle;
    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_SETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    validate_fattr(attr_stat->attributes, NFS__FTYPE__NFDIR);

    Nfs__FAttr *fattr = attr_stat->attributes;
    cr_assert_eq(fattr->atime->seconds, sattr.atime->seconds);
    cr_assert_eq(fattr->atime->useconds, sattr.atime->useconds);
    cr_assert_eq(fattr->mtime->seconds, sattr.mtime->seconds);
    cr_assert_eq(fattr->mtime->useconds, sattr.mtime->useconds);

    cr_assert_eq(((fattr->mode) & ((1<<12)-1)), sattr.mode); // only check first 12 bits - (sticky,setuid,setgid),owner,group,others, before that is device/file type
    cr_assert_eq(fattr->uid, sattr.uid);
    cr_assert_eq(fattr->gid, sattr.gid);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

Test(nfs_test_suite, setattr_no_such_file_or_directory, .description = "NFSPROC_SETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // pick a nonexistent inode number in this mounted directory and try to update its attributes
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = 0;
    sattr.uid = 0;   // make it a root-owned file
    sattr.gid = 0;
    sattr.size = 10;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = &fhandle;
    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attr_stat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attr_stat);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(attr_stat);

        cr_fail("NFSPROC_SETATTR failed - status %d\n", status);
    }

    cr_assert_eq(attr_stat->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(attr_stat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attr_stat->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

// NFSPROC_LOOKUP (4)
Test(nfs_test_suite, lookup_ok, .description = "NFSPROC_LOOKUP ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_test_suite, lookup_no_such_directory, .description = "NFSPROC_LOOKUP no such directory") {
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

Test(nfs_test_suite, lookup_no_such_file_or_directory, .description = "NFSPROC_LOOKUP no such file or directory") {
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

// NFSPROC_READ (6)
Test(nfs_test_suite, read_ok, .description = "NFSPROC_READ ok") {
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

Test(nfs_test_suite, read_no_such_file, .description = "NFSPROC_READ no such file") {
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

Test(nfs_test_suite, read_is_directory, .description = "NFSPROC_READ directory specified for a non directory operation") {
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

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_ok, .description = "NFSPROC_READDIR ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    int expected_number_of_entries = 5;
    char *expected_file_names[5] = {"..", ".", "dir1", "a.txt", "test_file.txt"};

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 1);    // we try and read all directory entries in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    for(int i = 0; i < expected_number_of_entries; i++) {
        cr_assert_not_null(directory_entries_head->name);
        cr_assert_str_eq(directory_entries_head->name->filename, expected_file_names[i], "Expected '%s' but found '%s'", expected_file_names[i], directory_entries_head->name->filename);
        cr_assert_not_null(directory_entries_head->cookie);

        if(i < expected_number_of_entries - 1) {
            cr_assert_not_null(directory_entries_head->nextentry);
        }
        else{
            cr_assert_null(directory_entries_head->nextentry);
        }

        directory_entries_head = directory_entries_head->nextentry;
    }

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_no_such_directory, .description = "NFSPROC_READDIR no such directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to read from a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_not_a_directory, .description = "NFSPROC_READDIR not a directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle root_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    root_fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&root_fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to readdir this test_file.txt (non-directory)
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &file_fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFSERR_NOTDIR);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_ok_lookup_then_readdir, .description = "NFSPROC_READDIR ok lookup then readdir") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup directory dir1 inside the mounted directory
    Nfs__FHandle root_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    root_fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&root_fhandle, "dir1", NFS__FTYPE__NFDIR);

    // try to readdir this directory dir1
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &dir_fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    // validate ReadDirRes
    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 1);    // we try and read all directory entries in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, "..", "Expected '..' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_not_null(directory_entries_head->nextentry);

    directory_entries_head = directory_entries_head->nextentry;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, ".", "Expected '.' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_null(directory_entries_head->nextentry);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_only_first_directory_entry, .description = "NFSPROC_READDIR ok read only first directory entry") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 20; // read only one directory entry

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 0);    // we try and read only the first directory entry in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, "..", "Expected '..' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_null(directory_entries_head->nextentry);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_directory_entries_in_batches, .description = "NFSPROC_READDIR ok read directory entries in batches") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    int expected_number_of_entries = 5;
    char *expected_file_names[5] = {"..", ".", "dir1", "a.txt", "test_file.txt"};

    long posix_cookie = 0;  // start from beginning of the directory stream
    int eof = 0;
    int index = 0;
    while(eof != 1 && index < expected_number_of_entries) {
        Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
        cookie.value = posix_cookie;

        Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
        readdirargs.dir = &fhandle;
        readdirargs.cookie = &cookie;
        readdirargs.count = 30; // aim to read only one directory entry

        Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
        int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
        if(status != 0) {
            mount__fh_status__free_unpacked(fhstatus, NULL);

            free(readdirres);

            cr_fail("NFSPROC_READDIR failed - status %d\n", status);
        }

        // validate ReadDirRes
        cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
        cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
        cr_assert_not_null(readdirres->readdirok);

        cr_assert_not_null(readdirres->readdirok->entries);
        eof = readdirres->readdirok->eof;

        Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
        Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
        while(directory_entries_head != NULL && index < expected_number_of_entries) {
            cr_assert_not_null(directory_entries_head->name);
            cr_assert_str_eq(directory_entries_head->name->filename, expected_file_names[index], "Expected '%s' but found '%s'", expected_file_names[index], directory_entries_head->name->filename);
            cr_assert_not_null(directory_entries_head->cookie);

            // update the cookie so that the next NFSPROC_READDIR call starts reading directories from where the previous call left off
            posix_cookie = directory_entries_head->cookie->value;

            if(index == expected_number_of_entries-1) {
                cr_assert_null(directory_entries_head->nextentry);
            }

            directory_entries_head = directory_entries_head->nextentry;
            index++;
        }

        nfs__read_dir_res__free_unpacked(readdirres, NULL);
    }

    cr_assert_eq(eof, 1);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}