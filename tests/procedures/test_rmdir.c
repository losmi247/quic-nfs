#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
 * NFSPROC_RMDIR (15) tests
 */

TestSuite(nfs_rmdir_test_suite);

Test(nfs_rmdir_test_suite, rmdir_ok, .description = "NFSPROC_RMDIR ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("rmdir_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the rmdir_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *rmdir_test_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "rmdir_test", NFS__FTYPE__NFDIR);

    // lookup the rmdir_test_dir directory that we are going to delete inside this directory - this will create an inode
    // mapping for it in the inode cache
    Nfs__FHandle rmdir_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle rmdir_test_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(rmdir_test_diropres->diropok->file->nfs_filehandle);
    rmdir_test_fhandle.nfs_filehandle = &rmdir_test_nfs_filehandle_copy;

    Nfs__DirOpRes *rmdir_test_dir_diropres = lookup_file_or_directory_success(
        rpc_connection_context, &rmdir_test_fhandle, "rmdir_test_dir", NFS__FTYPE__NFDIR);

    // delete the directory rmdir_test_dir inside this /nfs_share/rmdir_test directory
    Nfs__NfsStat *nfsstat = remove_directory_success(rpc_connection_context, &rmdir_test_fhandle, "rmdir_test_dir");
    nfs__dir_op_res__free_unpacked(rmdir_test_diropres, NULL);

    // read all entries in this directory rmdir_test to ensure rmdir_test_dir was deleted
    int expected_number_of_entries = 2;
    char *expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(rpc_connection_context, &rmdir_test_fhandle, 0, 1000,
                                                              expected_number_of_entries, expected_filenames);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);

    // try to read from the deleted directory to ensure its inode mapping was deleted from the inode cache
    read_from_directory_fail(rpc_connection_context, rmdir_test_dir_diropres->diropok->file, 0, 30,
                             NFS__STAT__NFSERR_NOENT);

    nfs__dir_op_res__free_unpacked(rmdir_test_dir_diropres, NULL);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_rmdir_test_suite, rmdir_no_such_containing_directory,
     .description = "NFSPROC_RMDIR no such containing directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("rmdir_no_such_containing_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to remove a directory 'rmdir_test_dir' inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    remove_directory_fail(rpc_connection_context, &fhandle, "rmdir_test_dir", NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_rmdir_test_suite, rmdir_directory_in_a_non_directory,
     .description = "NFSPROC_RMDIR rmdir a directory in a non-directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("rmdir_no_such_containing_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres =
        lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to remove a directory 'rmdir_test_dir' inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy =
        deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    remove_directory_fail(rpc_connection_context, &file_fhandle, "rmdir_test_dir", NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_rmdir_test_suite, rmdir_no_such_directory_to_be_deleted,
     .description = "NFSPROC_RMDIR no such directory to be deleted") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("rmdir_no_such_directory_to_be_deleted: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to remove a nonexistent directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_directory_fail(rpc_connection_context, &fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_NOENT);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_rmdir_test_suite, rmdir_not_a_directory,
     .description = "NFSPROC_RMDIR non-directory specified for a directory operation") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("rmdir_not_a_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to rmdir the test_file.txt from the mounted directory using RMDIR
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    remove_directory_fail(rpc_connection_context, &fhandle, "test_file.txt", NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

/*
 * Permission tests
 */

Test(nfs_rmdir_test_suite, rmdir_no_write_permission_on_containing_directory,
     .description = "NFSPROC_RMDIR no write permission on containing directory") {
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

    // now try to remove the directory "remove_dir1" inside this /nfs_share/permission_test/only_owner_write directory,
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
        cr_fatal("rmdir_no_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // fail since you don't have write permission on containing directory
    remove_directory_fail(rpc_connection_context, &only_owner_write_fhandle, "remove_dir1", NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_rmdir_test_suite, rmdir_has_write_permission_on_containing_directory,
     .description = "NFSPROC_RMDIR has write permission on containing directory") {
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

    // remove the directory "remove_dir2" inside this /nfs_share/permission_test/only_owner_write directory
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
        cr_fatal("rmdir_has_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // succeed since you have write permission on containing directory
    Nfs__NfsStat *nfsstat = remove_directory_success(rpc_connection_context, &only_owner_write_fhandle, "remove_dir2");
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}