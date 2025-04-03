#include "tests/test_common.h"

/*
 * NFSPROC_STATFS (17) tests
 */

TestSuite(nfs_statfs_test_suite);

Test(nfs_statfs_test_suite, statfs_ok, .description = "NFSPROC_STATFS ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("statfs_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // get filesystem attributes
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__StatFsRes *statfsres = get_filesystem_attributes_success(rpc_connection_context, fhandle);

    nfs__stat_fs_res__free_unpacked(statfsres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_statfs_test_suite, statfs_no_such_file_or_directory,
     .description = "NFSPROC_STATFS no such file or directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if (rpc_connection_context == NULL) {
        cr_fatal("statfs_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // pick a nonexistent inode number in this mounted directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    get_filesystem_attributes_fail(rpc_connection_context, fhandle, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}
