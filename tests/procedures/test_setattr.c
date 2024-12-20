#include "tests/test_common.h"

/*
* NFSPROC_SETATTR (2) tests
*/

TestSuite(nfs_setattr_test_suite);

Test(nfs_setattr_test_suite, setattr_ok, .description = "NFSPROC_SETATTR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // now update attributes of /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    
    Nfs__AttrStat *attrstat = set_attributes_success(NULL, &fhandle, 0, 0, 0, -1, &atime, &mtime, NFS__FTYPE__NFDIR);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

Test(nfs_setattr_test_suite, setattr_no_such_file_or_directory, .description = "NFSPROC_SETATTR no such file or directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // pick a nonexistent inode number in this mounted directory and try to update its attributes
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

    set_attributes_fail(NULL, &fhandle, 0, 0, 0, 10, &atime, &mtime, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Permission tests
*/

Test(nfs_setattr_test_suite, setattr_change_ownership_without_root_privileges, .description = "NFSPROC_SETATTR change ownership without root privileges") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // now try to update attributes of /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;
    
    // try to change ownership of this /nfs_share directory without root privileges (only root should be able to change ownership of files)
    uint32_t gids[1] = {1000};
    Rpc__OpaqueAuth *non_root_credential = create_auth_sys_opaque_auth("test", 1000, 1000, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_root_credential, verifier);

    set_attributes_fail(rpc_connection_context, &fhandle, 0, 1000, 1000, -1, &atime, &mtime, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_setattr_test_suite, setattr_no_write_permission, .description = "NFSPROC_SETATTR no write permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the setattr_only_owner_write.txt file inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *setattr_only_owner_write_file_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "setattr_only_owner_write.txt", NFS__FTYPE__NFREG);

    // now try to update attributes of this 'only_owner_write.txt' (other than uid/gid, so that we don't change ownership i.e. don't need root privileges)
    Nfs__FHandle setattr_only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle setattr_only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(setattr_only_owner_write_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(setattr_only_owner_write_file_diropres, NULL);
    setattr_only_owner_write_fhandle.nfs_filehandle = &setattr_only_owner_write_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier);

    // fail since you don't have write permission
    set_attributes_fail(rpc_connection_context, &setattr_only_owner_write_fhandle, -1, -1, -1, -1, &atime, &mtime, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_setattr_test_suite, setattr_has_write_permission, .description = "NFSPROC_SETATTR has write permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the setattr_only_owner_write.txt file inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *setattr_only_owner_write_file_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "setattr_only_owner_write.txt", NFS__FTYPE__NFREG);

    // update attributes of this 'setattr_only_owner_write.txt' (other than uid/gid, so that we don't change ownership i.e. don't need root privileges, only write permission)
    Nfs__FHandle setattr_only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle setattr_only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(setattr_only_owner_write_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(setattr_only_owner_write_file_diropres, NULL);
    setattr_only_owner_write_fhandle.nfs_filehandle = &setattr_only_owner_write_nfs_filehandle_copy;

    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = 200;
    atime.useconds = 0;
    mtime.seconds = 200;
    mtime.useconds = 0;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_GID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier);

    // succeed because you are the owner and have write permission
    Nfs__AttrStat *attrstat = set_attributes_success(rpc_connection_context, &setattr_only_owner_write_fhandle, -1, -1, -1, -1, &atime, &mtime, NFS__FTYPE__NFREG);
    nfs__attr_stat__free_unpacked(attrstat, NULL);

    free_rpc_connection_context(rpc_connection_context);
}