#include "tests/test_common.h"

/*
 * NFSPROC_READ (6) tests
 */

TestSuite(nfs_read_test_suite);

Test(nfs_read_test_suite, read_ok, .description = "NFSPROC_READ ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // read from this test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    uint8_t *expected_test_file_content = "test_content";
    uint8_t *expected_read_content = expected_test_file_content + 2;
    int expected_read_size = strlen(expected_read_content);
    Nfs__ReadRes *readres =
        read_from_file_success(rpc_connection_context, &file_fhandle, 2, expected_read_size,
                               diropres->diropok->attributes, expected_read_size, expected_read_content);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    nfs__read_res__free_unpacked(readres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_read_test_suite, read_no_such_file, .description = "NFSPROC_READ no such file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_no_such_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to read from a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    file_nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

    read_from_file_fail(rpc_connection_context, &file_fhandle, 2, 10, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_read_test_suite, read_is_directory,
     .description = "NFSPROC_READ directory specified for a non-directory operation") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_is_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to read from the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    read_from_file_fail(rpc_connection_context, &fhandle, 2, 10, NFS__STAT__NFSERR_ISDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_read_test_suite, read_more_than_maxdata_bytes, .description = "NFSPROC_READ read more than maxdata bytes") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_more_than_maxdata_bytes: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the large_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "large_file.txt", NFS__FTYPE__NFREG);

    // try to read NFS_MAXDATA + 20 bytes from this large_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    uint8_t *expected_read_content = malloc(sizeof(uint8_t) * NFS_MAXDATA); // expect to read NFS_MAXDATA zeros
    memset(expected_read_content, 0, NFS_MAXDATA);
    Nfs__ReadRes *readres = read_from_file_success(rpc_connection_context, &file_fhandle, 0, NFS_MAXDATA + 20,
                                                   diropres->diropok->attributes, NFS_MAXDATA, expected_read_content);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    nfs__read_res__free_unpacked(readres, NULL);

    free(expected_read_content);

    free_rpc_connection_context(rpc_connection_context);
}

/*
 * Permission tests
 */

Test(nfs_read_test_suite, read_no_read_permission, .description = "NFSPROC_READ no read permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup a file 'only_owner_read.txt' inside this /nfs_share/permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_read_file_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_read.txt", NFS__FTYPE__NFREG);

    // now try to read from this 'only_owner_read.txt' file, without having read permissions on it
    Nfs__FHandle only_owner_read_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_read_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_read_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_read_file_diropres, NULL);
    only_owner_read_fhandle.nfs_filehandle = &only_owner_read_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential =
        create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(
        non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_no_read_permission: Failed to connect to the server\n");
    }

    // fail since you don't have read permission on the file
    read_from_file_fail(rpc_connection_context, &only_owner_read_fhandle, 0, 10, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_read_test_suite, read_has_read_permission, .description = "NFSPROC_READ has read permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup a file 'only_owner_read.txt' inside this /nfs_share/permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_read_file_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_read.txt", NFS__FTYPE__NFREG);

    // now read from this 'only_owner_read.txt' file
    Nfs__FHandle only_owner_read_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_read_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_read_file_diropres->diropok->file->nfs_filehandle);
    only_owner_read_fhandle.nfs_filehandle = &only_owner_read_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_GID};
    Rpc__OpaqueAuth *owner_credential =
        create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context =
        create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("read_has_read_permission: Failed to connect to the server\n");
    }

    // succeed since you have read permission on containing directory
    Nfs__ReadRes *readres = read_from_file_success(rpc_connection_context, &only_owner_read_fhandle, 0, 8,
                                                   only_owner_read_file_diropres->diropok->attributes, 8, "testdata");
    nfs__read_res__free_unpacked(readres, NULL);

    free_rpc_connection_context(rpc_connection_context);

    nfs__dir_op_res__free_unpacked(only_owner_read_file_diropres, NULL);
}