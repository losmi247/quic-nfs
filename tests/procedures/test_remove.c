#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
 * NFSPROC_REMOVE (10) tests
 */

TestSuite(nfs_remove_test_suite);

Test(nfs_remove_test_suite, remove_ok, .description = "NFSPROC_REMOVE ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("remove_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the remove_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *remove_test_dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "remove_test", NFS__FTYPE__NFDIR);

    // lookup the remove_test_file.txt that we are going to delete inside this directory - this will create an inode
    // mapping for it in the inode cache
    Nfs__FHandle remove_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle remove_test_dir_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(remove_test_dir_diropres->diropok->file->nfs_filehandle);
    remove_test_dir_fhandle.nfs_filehandle = &remove_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *remove_test_file_diropres = lookup_file_or_directory_success(
        rpc_connection_context, &remove_test_dir_fhandle, "remove_test_file.txt", NFS__FTYPE__NFREG);

    // remove the file remove_test_file.txt inside this /nfs_share/remove_test directory
    Nfs__NfsStat *nfsstat = remove_file_success(NULL, &remove_test_dir_fhandle, "remove_test_file.txt");
    nfs__dir_op_res__free_unpacked(remove_test_dir_diropres, NULL);

    // read all entries in this directory remove_test to ensure remove_test_file.txt was deleted
    int expected_number_of_entries = 2;
    char *expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(rpc_connection_context, &remove_test_dir_fhandle, 0, 1000,
                                                              expected_number_of_entries, expected_filenames);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);

    // try to read from the deleted file to ensure its inode mapping was deleted from the inode cache
    read_from_file_fail(rpc_connection_context, remove_test_file_diropres->diropok->file, 0, 10,
                        NFS__STAT__NFSERR_NOENT);

    nfs__dir_op_res__free_unpacked(remove_test_file_diropres, NULL);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_remove_test_suite, remove_no_such_directory, .description = "NFSPROC_REMOVE no such directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("remove_no_such_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to remove a file 'remove_test_file.txt' inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    remove_file_fail(rpc_connection_context, &fhandle, "remove_file_test.txt", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_remove_test_suite, remove_file_in_a_non_directory,
     .description = "NFSPROC_REMOVE remove file in a non-directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("remove_file_in_a_non_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to remove a file "remove_test_file.txt" inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    remove_file_fail(rpc_connection_context, &file_fhandle, "remove_test_file.txt", NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_remove_test_suite, remove_no_such_file, .description = "NFSPROC_REMOVE no such file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("remove_no_such_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to remove a nonexistent file
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_file_fail(rpc_connection_context, &fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_NOENT);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_remove_test_suite, remove_is_directory,
     .description = "NFSPROC_REMOVE directory specified for a non-directory operation") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("remove_is_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to remove the remove_test directory from the mounted directory using REMOVE
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_file_fail(rpc_connection_context, &fhandle, "remove_test", NFS__STAT__NFSERR_ISDIR);

    free_rpc_connection_context(rpc_connection_context);
}

/*
 * Permission tests
 */

Test(nfs_remove_test_suite, remove_no_write_permission_on_containing_directory,
     .description = "NFSPROC_REMOVE no write permission on containing directory") {
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

    // now try to remove the file "remove_file1.txt" inside this /nfs_share/permission_test/only_owner_write directory,
    // without having write permission on that directory
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
        cr_fatal("remove_no_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // fail since you don't have write permission on containing directory
    remove_file_fail(rpc_connection_context, &only_owner_write_fhandle, "remove_file1.txt", NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_remove_test_suite, remove_has_write_permission_on_containing_directory,
     .description = "NFSPROC_REMOVE has write permission on containing directory") {
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

    // remove the file "remove_file2.txt" inside this /nfs_share/permission_test/only_owner_write directory
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
        cr_fatal("remove_has_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // succeed since you have write permission on containing directory
    Nfs__NfsStat *nfsstat = remove_file_success(rpc_connection_context, &only_owner_write_fhandle, "remove_file2.txt");
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}