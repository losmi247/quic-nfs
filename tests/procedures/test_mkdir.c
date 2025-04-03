#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
 * NFSPROC_MKDIR (14) tests
 */

TestSuite(nfs_mkdir_test_suite);

Test(nfs_mkdir_test_suite, mkdir_ok, .description = "NFSPROC_MKDIR ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // create a directory mkdir_test_dir inside this /nfs_share/create_test directory
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(mkdir_test_dir_diropres, NULL);
    mkdir_test_dir_fhandle.nfs_filehandle = &mkdir_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    Nfs__DirOpRes *diropres =
        create_directory_success(rpc_connection_context, &mkdir_test_dir_fhandle, "mkdir_test_dir", 0440, 0, 0, &atime,
                                 &mtime, NFS__FTYPE__NFDIR); // root-owned file (uid=gid=0)

    nfs__dir_op_res__free_unpacked(diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_mkdir_test_suite, mkdir_no_such_directory, .description = "NFSPROC_MKDIR no such directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_no_such_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

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
    create_directory_fail(rpc_connection_context, &fhandle, "mkdir_test_dir", 0440, 0, 0, &atime, &mtime,
                          NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_mkdir_test_suite, mkdir_directory_in_a_non_directory,
     .description = "NFSPROC_MKDIR create directory in a non-directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_directory_in_a_non_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a directory mkdir_test_dir inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(rpc_connection_context, &file_fhandle, "mkdir_test_dir", 0440, 0, 0, &atime, &mtime,
                          NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_mkdir_test_suite, mkdir_file_name_too_long, .description = "NFSPROC_MKDIR file name too long") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_file_name_too_long: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the mkdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // try to create a directory with a too long filename inside this /nfs_share/mkdir_test directory
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
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
    create_directory_fail(rpc_connection_context, &mkdir_test_dir_fhandle, filename, 0440, 0, 0, &atime, &mtime,
                          NFS__STAT__NFSERR_NAMETOOLONG);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_mkdir_test_suite, mkdir_already_existing_directory,
     .description = "NFSPROC_MKDIR create already existing directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_already_existing_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the mkdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *mkdir_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "mkdir_test", NFS__FTYPE__NFDIR);

    // try to create a file /nfs_share/mkdir_test/existing_directory that already exists
    Nfs__FHandle mkdir_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle mkdir_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(mkdir_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(mkdir_test_dir_diropres, NULL);
    mkdir_test_dir_fhandle.nfs_filehandle = &mkdir_test_dir_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    create_directory_fail(rpc_connection_context, &mkdir_test_dir_fhandle, "existing_directory", 0440, 0, 0, &atime,
                          &mtime, NFS__STAT__NFSERR_EXIST);

    free_rpc_connection_context(rpc_connection_context);
}

/*
 * Permission tests
 */

Test(nfs_mkdir_test_suite, mkdir_no_write_permission_on_containing_directory,
     .description = "NFSPROC_MKDIR no write permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_write directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_dir_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write", NFS__FTYPE__NFDIR);

    // now try to create a directory inside this /nfs_share/permission_test/only_owner_write directory, without having
    // write permission on that directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential =
        create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(
        non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_no_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = 0;

    // fail since you don't have write permission on the containing directory
    create_directory_fail(rpc_connection_context, &only_owner_write_fhandle, NONEXISTENT_FILENAME, 0, 0, 0, &atime,
                          &mtime, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_mkdir_test_suite, mkdir_has_write_permission_on_containing_directory,
     .description = "NFSPROC_MKDIR has write permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_write directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_dir_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write", NFS__FTYPE__NFDIR);

    // create a directory inside this /nfs_share/permission_test/only_owner_write directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *owner_credential =
        create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context =
        create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("mkdir_has_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = 0;

    // succeed since you have write permission on containing directories
    Nfs__DirOpRes *diropres = create_directory_success(rpc_connection_context, &only_owner_write_fhandle,
                                                       "create_directory", 0, 0, 0, &atime, &mtime, NFS__FTYPE__NFDIR);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}