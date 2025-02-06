#include "tests/test_common.h"

/*
* NFSPROC_READLINK (5) tests
*/

TestSuite(nfs_readlink_test_suite);

Test(nfs_readlink_test_suite, readlink_ok, .description = "NFSPROC_READLINK ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readlink_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the readlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *readlink_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "readlink_test", NFS__FTYPE__NFDIR);

    // lookup the 'symlink' symbolic link inside this /nfs_share/readlink_test directory
    Nfs__FHandle readlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle readlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(readlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(readlink_test_dir_diropres, NULL);
    readlink_test_dir_fhandle.nfs_filehandle = &readlink_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_diropres = lookup_file_or_directory_success(rpc_connection_context, &readlink_test_dir_fhandle, "symlink", NFS__FTYPE__NFLNK);

    // read the pathname inside this symbolic link 'symlink'
    Nfs__FHandle symlink_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_diropres, NULL);
    symlink_fhandle.nfs_filehandle = &symlink_nfs_filehandle_copy;

    char *expected_symlink_target_path = "/nfs_share/readlink_test/target_file.txt";
    Nfs__ReadLinkRes *readlinkres = read_from_symbolic_link_success(rpc_connection_context, &symlink_fhandle, expected_symlink_target_path);
    nfs__read_link_res__free_unpacked(readlinkres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readlink_test_suite, readlink_no_such_file, .description = "NFSPROC_READLINK no such file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readlink_no_such_file: Failed to connect to the server\n");
    }
    
    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to read from a nonexistent symbolic link
    NfsFh__NfsFileHandle symlink_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    symlink_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    symlink_nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle symlink_fhandle = NFS__FHANDLE__INIT;
    symlink_fhandle.nfs_filehandle = &symlink_nfs_filehandle;

    read_from_symbolic_link_fail(rpc_connection_context, &symlink_fhandle, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readlink_test_suite, readlink_not_a_symboliclink, .description = "NFSPROC_READLINK specified file which is not a symbolic link") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readlink_no_such_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the readlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *readlink_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "readlink_test", NFS__FTYPE__NFDIR);

    // try to readlink this symlink_test directory
    Nfs__FHandle readlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle readlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(readlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(readlink_test_dir_diropres, NULL);
    readlink_test_dir_fhandle.nfs_filehandle = &readlink_test_dir_nfs_filehandle_copy;

    read_from_symbolic_link_fail(rpc_connection_context, &readlink_test_dir_fhandle, NFS__STAT__NFSERR_STALE);

    free_rpc_connection_context(rpc_connection_context);
}