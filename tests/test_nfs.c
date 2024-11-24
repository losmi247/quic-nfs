#include "test_common.h"

#include <stdlib.h>
#include <stdio.h>

/*
* Mount RPC program tests.
*/

TestSuite(mount_test_suite);

Test(mount_test_suite, null, .description = "MOUNTPROC_NULL") {
    int status = mount_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

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
    cr_assert_neq(fhstatus->default_case, NULL);

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
    cr_assert_neq(fhstatus->default_case, NULL);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Nfs RPC program tests.
*/

TestSuite(nfs_test_suite);

Test(nfs_test_suite, null, .description = "NFSPROC_NULL") {
    int status = nfs_procedure_0_do_nothing();
    if (status != 0) {
        cr_fail("MOUNTPROC_NULL failed - status %d\n", status);
    }
}

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
    cr_assert_neq(attr_stat->default_case, NULL);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

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
    cr_assert_neq(attr_stat->default_case, NULL);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__attr_stat__free_unpacked(attr_stat, NULL);
}

Test(nfs_test_suite, lookup_ok, .description = "NFSPROC_LOOKUP ok") {
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

    cr_assert_eq(diropres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DIROPOK);
    cr_assert_neq(diropres->diropok, NULL);

    cr_assert_neq(diropres->diropok->file, NULL);
    cr_assert_neq(diropres->diropok->file->nfs_filehandle, NULL);     // can't validate NFS filehandle contents as a client
    validate_fattr(diropres->diropok->attributes, NFS__FTYPE__NFREG); // can't validate any other attributes

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_test_suite, lookup_no_such_directory, .description = "NFSPROC_LOOKUP no such directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup a test_file.txt inside a different nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

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
    cr_assert_neq(diropres->default_case, NULL);

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
    cr_assert_neq(diropres->default_case, NULL);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
}