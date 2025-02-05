#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_CREATE (9) tests
*/

TestSuite(nfs_create_test_suite);

Test(nfs_create_test_suite, create_ok, .description = "NFSPROC_CREATE ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "create_test", NFS__FTYPE__NFDIR);

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
    Nfs__DirOpRes *diropres = create_file_success(rpc_connection_context, &create_test_dir_fhandle, "create_test_file.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__FTYPE__NFREG); // root-owned file (uid=gid=0)

    nfs__dir_op_res__free_unpacked(diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_create_test_suite, create_no_such_directory, .description = "NFSPROC_CREATE no such directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_no_such_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

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
    create_file_fail(rpc_connection_context, &fhandle, "create_test.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_create_test_suite, create_file_in_a_non_directory, .description = "NFSPROC_CREATE create file in a non-directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_file_in_a_non_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

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
    create_file_fail(rpc_connection_context, &file_fhandle, "create_test.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_create_test_suite, create_file_name_too_long, .description = "NFSPROC_CREATE file name too long") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_file_name_too_long: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "create_test", NFS__FTYPE__NFDIR);

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
    create_file_fail(rpc_connection_context, &create_test_dir_fhandle, filename, 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NAMETOOLONG);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_create_test_suite, create_already_existing_file, .description = "NFSPROC_CREATE create already existing file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_already_existing_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the create_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *create_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "create_test", NFS__FTYPE__NFDIR);

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
    create_file_fail(rpc_connection_context, &create_test_dir_fhandle, "existing_file.txt", 0440, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_EXIST);

    free_rpc_connection_context(rpc_connection_context);
}

/*
* Permission tests
*/

Test(nfs_create_test_suite, create_no_write_permission_on_containing_directory, .description = "NFSPROC_CREATE no write permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_write directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write", NFS__FTYPE__NFDIR);

    // now try to create a file inside this /nfs_share/permission_test/only_owner_write directory, without having write permission on that directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_no_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = 0;

    // fail since you don't have write permission on containing directory
    create_file_fail(rpc_connection_context, &only_owner_write_fhandle, NONEXISTENT_FILENAME, 0, 0, 0, 0, &atime, &mtime, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_create_test_suite, create_has_write_permission_on_containing_directory, .description = "NFSPROC_CREATE has write permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_write directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write", NFS__FTYPE__NFDIR);

    // create a file 'file.txt' inside this /nfs_share/permission_test/only_owner_write directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_has_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = 0;

    // succeed since you have write permission on containing directory
    Nfs__DirOpRes *diropres = create_file_success(rpc_connection_context, &only_owner_write_fhandle, "create_file.txt", 0, 0, 0, 0, &atime, &mtime, NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}