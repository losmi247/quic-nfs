#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_SYMLINK (13) tests
*/

TestSuite(nfs_symlink_test_suite);

Test(nfs_symlink_test_suite, symlink_ok, .description = "NFSPROC_SYMLINK ok") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "symlink_test", NFS__FTYPE__NFDIR);

    // create a symbolic link 'symlink' to /nfs_share/symlink_test/existing_file, inside this /nfs_share/symlink_test directory
    Nfs__FHandle symlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_test_dir_diropres, NULL);
    symlink_test_dir_fhandle.nfs_filehandle = &symlink_test_dir_nfs_filehandle_copy;

    Nfs__Path target = NFS__PATH__INIT;
    target.path = "/nfs_share/symlink_test/existing_file";
    Nfs__NfsStat *nfsstat = create_symbolic_link_success(NULL, &symlink_test_dir_fhandle, "symlink", &target);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    // lookup the created 'symlink' file to verify its file type is NFLNK
    Nfs__DirOpRes *symlink_diropres = lookup_file_or_directory_success(NULL, &symlink_test_dir_fhandle, "symlink", NFS__FTYPE__NFLNK);

    // read from the created symbolic link to verify the correct path was written into it
    Nfs__FHandle symlink_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_diropres, NULL);
    symlink_fhandle.nfs_filehandle = &symlink_nfs_filehandle_copy;

    Nfs__ReadLinkRes *readlinkres = read_from_symbolic_link_success(NULL, &symlink_fhandle, target.path);
    nfs__read_link_res__free_unpacked(readlinkres, NULL);
}

Test(nfs_symlink_test_suite, symlink_no_such_directory, .description = "NFSPROC_SYMLINK no such directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // try to create a symbolic link 'symlink' (to some target) inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";

    create_symbolic_link_fail(NULL, &fhandle, "symlink", &path, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_symlink_test_suite, symlink_create_a_symbolic_link_in_a_non_directory, .description = "NFSPROC_SYMLINK create symbolic link in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a symbolic link "symlink" (to some target) inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";

    create_symbolic_link_fail(NULL, &file_fhandle, "symlink", &path, NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_symlink_test_suite, symlink_file_name_too_long, .description = "NFSPROC_SYMLINK file name too long") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "symlink_test", NFS__FTYPE__NFDIR);

    // try to create a symbolic link (to some target) with a too long filename inside this /nfs_share/symlink_test directory
    Nfs__FHandle symlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_test_dir_diropres, NULL);
    symlink_test_dir_fhandle.nfs_filehandle = &symlink_test_dir_nfs_filehandle_copy;

    char *filename = malloc(sizeof(char) * (NFS_MAXNAMLEN + 20));
    memset(filename, 'a', NFS_MAXNAMLEN + 19);
    filename[NFS_MAXNAMLEN + 19] = '\0'; // null terminate the name

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";

    create_symbolic_link_fail(NULL, &symlink_test_dir_fhandle, filename, &path, NFS__STAT__NFSERR_NAMETOOLONG);
}

Test(nfs_symlink_test_suite, create_symbolic_link_that_is_an_already_existing_file, .description = "NFSPROC_SYMLINK create symbolic link that is an already existing file") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "symlink_test", NFS__FTYPE__NFDIR);

    // try to create a symbolic link (to some target) at /nfs_share/symlink_test/existing_file that already exists
    Nfs__FHandle symlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_test_dir_diropres, NULL);
    symlink_test_dir_fhandle.nfs_filehandle = &symlink_test_dir_nfs_filehandle_copy;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";
    
    create_symbolic_link_fail(NULL, &symlink_test_dir_fhandle, "existing_file", &path, NFS__STAT__NFSERR_EXIST);
}

/*
* Permission tests
*/

Test(nfs_symlink_test_suite, symlink_no_write_permission_on_containing_directory, .description = "NFSPROC_SYMLINK no write permission on containing directory") {
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

    // now try to create a symbolic link inside this /nfs_share/permission_test/only_owner_write directory, without having write permission on that directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "path";

    // fail since you don't have write permission on the containing directory
    create_symbolic_link_fail(rpc_connection_context, &only_owner_write_fhandle, NONEXISTENT_FILENAME, &path, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_symlink_test_suite, symlink_has_write_permission_on_containing_directory, .description = "NFSPROC_SYMLINK has write permission on containing directory") {
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

    // create a symbolic link inside this /nfs_share/permission_test/only_owner_write directory
    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "path";

    // succeed since you have write permission on the containing directory
    Nfs__NfsStat *nfsstat = create_symbolic_link_success(rpc_connection_context, &only_owner_write_fhandle, "permissions_symlink", &path);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);

    // lookup the created 'permissions_symlink' file to verify its file type is NFLNK
    Nfs__DirOpRes *symlink_diropres = lookup_file_or_directory_success(NULL, &only_owner_write_fhandle, "permissions_symlink", NFS__FTYPE__NFLNK);

    // read from the created symbolic link to verify the correct path was written into it
    Nfs__FHandle symlink_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_diropres, NULL);
    symlink_fhandle.nfs_filehandle = &symlink_nfs_filehandle_copy;

    Nfs__ReadLinkRes *readlinkres = read_from_symbolic_link_success(NULL, &symlink_fhandle, path.path);
    nfs__read_link_res__free_unpacked(readlinkres, NULL);
}