#include "tests/test_common.h"
#include <stdio.h>
/*
* NFSPROC_GETATTR (1) tests
*/

TestSuite(nfs_getattr_test_suite);

Test(nfs_getattr_test_suite, getattr_ok, .description = "NFSPROC_GETATTR ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("getattr_ok: Failed to connect to the server\n");
    }
    
    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // now get file attributes for the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__AttrStat *attrstat = get_attributes_success(rpc_connection_context, fhandle, NFS__FTYPE__NFDIR);

    nfs__attr_stat__free_unpacked(attrstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_getattr_test_suite, getattr_no_such_file_or_directory, .description = "NFSPROC_GETATTR no such file or directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("getattr_no_such_file_or_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // pick a nonexistent inode number in this mounted directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    get_attributes_fail(rpc_connection_context, fhandle, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

