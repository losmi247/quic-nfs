#include "tests/test_common.h"

/*
* NFSPROC_LOOKUP (4) tests
*/

TestSuite(nfs_lookup_test_suite);

Test(nfs_lookup_test_suite, lookup_ok, .description = "NFSPROC_LOOKUP ok") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(NULL, &dir_fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

Test(nfs_lookup_test_suite, lookup_inside_non_existent_directory, .description = "NFSPROC_LOOKUP inside non-existent directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup a test_file.txt inside a different nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    lookup_file_or_directory_fail(NULL, &fhandle, "test_file.txt", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_lookup_test_suite, lookup_no_such_file_or_directory, .description = "NFSPROC_LOOKUP no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup a nonexistent file inside this mounted directory
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    lookup_file_or_directory_fail(NULL, &dir_fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_NOENT);
}

Test(nfs_lookup_test_suite, lookup_a_non_directory, .description = "NFSPROC_LOOKUP lookup a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to lookup a file name "a.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    lookup_file_or_directory_fail(NULL, &file_fhandle, "a.txt", NFS__STAT__NFSERR_NOTDIR);
}

/*
* Permission tests
*/

Test(nfs_lookup_test_suite, lookup_no_execute_permission_on_containing_directory, .description = "NFSPROC_LOOKUP no execute permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_execute directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_execute_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_execute", NFS__FTYPE__NFDIR);

    // now try to lookup a file 'file.txt' inside this /nfs_share/permission_test/only_owner_execute directory, without having execute permission
    Nfs__FHandle only_owner_execute_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_execute_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_execute_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_execute_dir_diropres, NULL);
    only_owner_execute_fhandle.nfs_filehandle = &only_owner_execute_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier);

    // fail since you don't have execute permission on containing directory
    lookup_file_or_directory_fail(rpc_connection_context, &only_owner_execute_fhandle, "file.txt", NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_lookup_test_suite, lookup_has_execute_permission_on_containing_directory, .description = "NFSPROC_LOOKUP has execute permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_execute directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_execute_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_execute", NFS__FTYPE__NFDIR);

    // now try to lookup a file 'file.txt' inside this /nfs_share/permission_test/only_owner_execute directory, without having execute permission
    Nfs__FHandle only_owner_execute_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_execute_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_execute_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_execute_dir_diropres, NULL);
    only_owner_execute_fhandle.nfs_filehandle = &only_owner_execute_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_GID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier);

    // succeed since you have read permission on containing directory
    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(rpc_connection_context, &only_owner_execute_fhandle, "file.txt", NFS__FTYPE__NFREG);
    nfs__dir_op_res__free_unpacked(diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}