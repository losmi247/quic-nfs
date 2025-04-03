#include "tests/test_common.h"

#include <stdio.h>

/*
 * NFSPROC_WRITE (8) tests
 */

TestSuite(nfs_write_test_suite);

Test(nfs_write_test_suite, write_ok, .description = "NFSPROC_WRITE ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the write_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *write_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "write_test", NFS__FTYPE__NFDIR);

    // lookup the write_test_file.txt inside this /nfs_share/write_test directory
    Nfs__FHandle write_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle write_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(write_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(write_test_dir_diropres, NULL);
    write_test_dir_fhandle.nfs_filehandle = &write_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(rpc_connection_context, &write_test_dir_fhandle,
                                                               "write_test_file.txt", NFS__FTYPE__NFREG);

    // write to this write_test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__AttrStat *attrstat =
        write_to_file_success(rpc_connection_context, &file_fhandle, 6, 4, "done", NFS__FTYPE__NFREG);

    // read from write_test_file.txt to confirm the write was successful
    uint8_t *expected_new_test_file_content = "write_done_content";
    uint8_t expected_read_size = strlen(expected_new_test_file_content);
    Nfs__ReadRes *readres =
        read_from_file_success(rpc_connection_context, &file_fhandle, 0, expected_read_size, attrstat->attributes,
                               expected_read_size, expected_new_test_file_content);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
    nfs__read_res__free_unpacked(readres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_write_test_suite, write_no_such_file, .description = "NFSPROC_WRITE no such file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_no_such_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to write to a nonexistent file
    NfsFh__NfsFileHandle file_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    file_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    file_nfs_filehandle.timestamp = 0;

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    file_fhandle.nfs_filehandle = &file_nfs_filehandle;

    write_to_file_fail(rpc_connection_context, &file_fhandle, 2, 5, "write", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_write_test_suite, write_is_directory,
     .description = "NFSPROC_WRITE directory specified for a non-directory operation") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_is_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to write to the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    write_to_file_fail(rpc_connection_context, &fhandle, 2, 5, "write", NFS__STAT__NFSERR_ISDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_write_test_suite, write_too_much_data, .description = "NFSPROC_WRITE more than NFS_MAXDATA bytes in nfsdata") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_too_much_data: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the write_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *write_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "write_test", NFS__FTYPE__NFDIR);

    // lookup the write_test_file.txt inside this /nfs_share/write_test directory
    Nfs__FHandle write_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle write_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(write_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(write_test_dir_diropres, NULL);
    write_test_dir_fhandle.nfs_filehandle = &write_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(rpc_connection_context, &write_test_dir_fhandle,
                                                               "write_test_file.txt", NFS__FTYPE__NFREG);

    // try to write 10000 bytes in a single RPC to this write_test_file.txt
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    uint8_t *content = malloc(sizeof(uint8_t) * (NFS_MAXDATA + 20));
    memset(content, 'a', NFS_MAXDATA + 20);
    write_to_file_fail(rpc_connection_context, &file_fhandle, 6, NFS_MAXDATA + 20, content, NFS__STAT__NFSERR_FBIG);

    free(content);

    free_rpc_connection_context(rpc_connection_context);
}

/*
 * Permission tests
 */

Test(nfs_write_test_suite, write_no_write_permission, .description = "NFSPROC_WRITE no write permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup a file 'only_owner_write1.txt' inside this /nfs_share/permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_file_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write1.txt", NFS__FTYPE__NFREG);

    // now try to write to this 'only_owner_write1.txt' file, without having write permissions on it
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_write_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_file_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential =
        create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(
        non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_no_write_permission: Failed to connect to the server\n");
    }

    // fail since you don't have read permission on the file
    write_to_file_fail(rpc_connection_context, &only_owner_write_fhandle, 0, 10, "writedata", NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_write_test_suite, write_has_write_permission, .description = "NFSPROC_WRITE has write permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres =
        lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup a file 'only_owner_write2.txt' inside this /nfs_share/permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_write_file_diropres =
        lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_write2.txt", NFS__FTYPE__NFREG);

    // write to this 'only_owner_write2.txt' file
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(only_owner_write_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_file_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_GID};
    Rpc__OpaqueAuth *owner_credential =
        create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context =
        create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("write_has_write_permission: Failed to connect to the server\n");
    }

    // succeed since you have read permission on containing directory
    Nfs__AttrStat *attrstat =
        write_to_file_success(rpc_connection_context, &only_owner_write_fhandle, 0, 9, "writedata", NFS__FTYPE__NFREG);
    free_rpc_connection_context(rpc_connection_context);

    // read from write_test_file2.txt to confirm the write was successful
    uint8_t *expected_new_file_content = "writedata";
    uint8_t expected_read_size = strlen(expected_new_file_content);
    Nfs__ReadRes *readres = read_from_file_success(NULL, &only_owner_write_fhandle, 0, expected_read_size,
                                                   attrstat->attributes, expected_read_size, expected_new_file_content);
    nfs__read_res__free_unpacked(readres, NULL);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}