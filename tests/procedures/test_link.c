#include "tests/test_common.h"

#include <stdio.h>
/*
* NFSPROC_LINK (12) tests
*/

TestSuite(nfs_link_test_suite);

Test(nfs_link_test_suite, link_ok, .description = "NFSPROC_LINK ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // create a hard link 'hardlink' to /nfs_share/link_test/existing_file, inside this /nfs_share/link_test directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    Nfs__NfsStat *nfsstat = create_link_to_file_success(rpc_connection_context, &target_file_fhandle, &link_test_dir_fhandle, "hardlink");
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    // lookup the created 'hardlink' file to verify its file type is NFREG, same as the target
    Nfs__DirOpRes *link_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "hardlink", NFS__FTYPE__NFREG);

    // read from the created hard link to confirm it works
    Nfs__FHandle link_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_diropres->diropok->file->nfs_filehandle);
    link_fhandle.nfs_filehandle = &link_nfs_filehandle_copy;

    Nfs__FAttr *attributes_before_read = link_diropres->diropok->attributes;

    uint8_t *expected_new_test_file_content = "existing_file_content";
    uint8_t expected_read_size = strlen(expected_new_test_file_content);
    Nfs__ReadRes *readres = read_from_file_success(rpc_connection_context, &link_fhandle, 0, expected_read_size, attributes_before_read, expected_read_size, expected_new_test_file_content);

    nfs__read_res__free_unpacked(readres, NULL);
    nfs__dir_op_res__free_unpacked(link_diropres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, link_no_such_target_file, .description = "NFSPROC_LINK no such target file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_no_such_target_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // try to create a hard link 'hardlink' to a nonexistent target inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    target_file_fhandle.nfs_filehandle = &nfs_filehandle;

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &link_test_dir_fhandle, "hardlink" , NFS__STAT__NFSERR_NOENT);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, link_target_file_is_a_directory, .description = "NFSPROC_LINK target file is a directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_target_file_is_a_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

     // lookup the 'existing_directory' directory inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_directory", NFS__FTYPE__NFDIR);

    // try to create a hard link 'hardlink' to the 'existing_directory' directory target inside this link_test directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &link_test_dir_fhandle, "hardlink" , NFS__STAT__NFSERR_ISDIR);

    free_rpc_connection_context(rpc_connection_context);
}


Test(nfs_link_test_suite, link_no_such_directory, .description = "NFSPROC_LINK no such directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_no_such_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // try to create a hard link 'hardlink' to some 'existing_file' inside a nonexistent directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    NfsFh__NfsFileHandle dir_nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    dir_nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    dir_nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle;

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &dir_fhandle, "hardlink", NFS__STAT__NFSERR_NOENT);
    
    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, link_create_a_hard_link_in_a_non_directory, .description = "NFSPROC_LINK create hard link in a non-directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_create_a_hard_link_in_a_non_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // lookup the test_file.txt inside the mounted directory
    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a hard link "hardlink" to the 'existing_file' inside this test_file.txt file
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &file_fhandle, "hardlink", NFS__STAT__NFSERR_NOTDIR);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, link_file_name_too_long, .description = "NFSPROC_LINK file name too long") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_file_name_too_long: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // try to create a hard link to 'existing_file' with a too long filename inside this /nfs_share/link_test directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    char *filename = malloc(sizeof(char) * (NFS_MAXNAMLEN + 20));
    memset(filename, 'a', NFS_MAXNAMLEN + 19);
    filename[NFS_MAXNAMLEN + 19] = '\0'; // null terminate the name

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &link_test_dir_fhandle, filename, NFS__STAT__NFSERR_NAMETOOLONG);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, create_hard_link_that_is_an_already_existing_file, .description = "NFSPROC_LINK create hard link that is an already existing file") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("create_hard_link_that_is_an_already_existing_file: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the link_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(rpc_connection_context, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // try to create a hard link 'existing_file' at /nfs_share/symlink_test/existing_file that already exists
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &link_test_dir_fhandle, "existing_file", NFS__STAT__NFSERR_EXIST);

    free_rpc_connection_context(rpc_connection_context);
}

/*
* Permission tests
*/

Test(nfs_link_test_suite, link_no_write_permission_on_containing_directory, .description = "NFSPROC_LINK no write permission on containing directory") {
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

    // lookup the link_test directory inside the mounted directory
    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(NULL, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // now try to create a hard link inside this /nfs_share/permission_test/only_owner_write directory, without having write permission on that directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_no_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // fail since you don't have write permission on the containing directory
    create_link_to_file_fail(rpc_connection_context, &target_file_fhandle, &only_owner_write_fhandle, NONEXISTENT_FILENAME, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_link_test_suite, link_has_write_permission_on_containing_directory, .description = "NFSPROC_LINK has write permission on containing directory") {
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

    // lookup the link_test directory inside the mounted directory
    Nfs__DirOpRes *link_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "link_test", NFS__FTYPE__NFDIR);

    // lookup the 'existing_file' file inside this link_test directory
    Nfs__FHandle link_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(link_test_dir_diropres, NULL);
    link_test_dir_fhandle.nfs_filehandle = &link_test_dir_nfs_filehandle_copy;

    Nfs__DirOpRes *target_file_diropres = lookup_file_or_directory_success(NULL, &link_test_dir_fhandle, "existing_file", NFS__FTYPE__NFREG);

    // now try to create a hard link inside this /nfs_share/permission_test/only_owner_write directory
    Nfs__FHandle target_file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle target_file_nfs_filehandle_copy = deep_copy_nfs_filehandle(target_file_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(target_file_diropres, NULL);
    target_file_fhandle.nfs_filehandle = &target_file_nfs_filehandle_copy;

    Nfs__FHandle only_owner_write_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_write_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_write_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_write_dir_diropres, NULL);
    only_owner_write_fhandle.nfs_filehandle = &only_owner_write_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("link_has_write_permission_on_containing_directory: Failed to connect to the server\n");
    }

    // succeed since you have write permission on the containing directory
    Nfs__NfsStat *nfsstat = create_link_to_file_success(rpc_connection_context, &target_file_fhandle, &only_owner_write_fhandle, "permissions_hardlink");
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    free_rpc_connection_context(rpc_connection_context);

    // lookup the created 'hardlink' file to verify its file type is NFREG, same as the target
    Nfs__DirOpRes *link_diropres = lookup_file_or_directory_success(NULL, &only_owner_write_fhandle, "permissions_hardlink", NFS__FTYPE__NFREG);

    // read from the created hard link to confirm it works
    Nfs__FHandle link_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle link_nfs_filehandle_copy = deep_copy_nfs_filehandle(link_diropres->diropok->file->nfs_filehandle);
    link_fhandle.nfs_filehandle = &link_nfs_filehandle_copy;

    Nfs__FAttr *attributes_before_read = link_diropres->diropok->attributes;

    uint8_t *expected_new_test_file_content = "existing_file_content";
    uint8_t expected_read_size = strlen(expected_new_test_file_content);
    Nfs__ReadRes *readres = read_from_file_success(NULL, &link_fhandle, 0, expected_read_size, attributes_before_read, expected_read_size, expected_new_test_file_content);

    nfs__read_res__free_unpacked(readres, NULL);
    nfs__dir_op_res__free_unpacked(link_diropres, NULL);
}