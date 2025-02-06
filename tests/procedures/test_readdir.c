#include "tests/test_common.h"

#include <stdlib.h>
#include <stdio.h>

/*
* NFSPROC_READDIR (16) tests
*/

TestSuite(nfs_readdir_test_suite);

Test(nfs_readdir_test_suite, readdir_ok, .description = "NFSPROC_READDIR ok") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_ok: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // read all entries from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    int expected_number_of_entries = NFS_SHARE_NUMBER_OF_ENTRIES;
    char *expected_filenames[NFS_SHARE_NUMBER_OF_ENTRIES] = NFS_SHARE_ENTRIES;

    Nfs__ReadDirRes *readdirres = read_from_directory_success(rpc_connection_context, &fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_no_such_directory, .description = "NFSPROC_READDIR no such directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_no_such_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // try to read from a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    read_from_directory_fail(rpc_connection_context, &fhandle, 0, 1000, NFS__STAT__NFSERR_NOENT);  // cookie=0, start from beginning of the directory stream

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_not_a_directory, .description = "NFSPROC_READDIR not a directory") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_not_a_directory: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to readdir this test_file.txt (non-directory)
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    read_from_directory_fail(rpc_connection_context, &file_fhandle, 0, 1000, NFS__STAT__NFSERR_NOTDIR);  // cookie=0, start from beginning of the directory stream

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_ok_lookup_then_readdir, .description = "NFSPROC_READDIR ok lookup then readdir") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_ok_lookup_then_readdir: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // lookup directory write_test inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(rpc_connection_context, &fhandle, "write_test", NFS__FTYPE__NFDIR);

    // read all entries in this directory write_test
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    int expected_number_of_entries = 3;
    char *expected_filenames[5] = {"..", ".", "write_test_file.txt"};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(rpc_connection_context, &dir_fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_ok_read_only_first_directory_entry, .description = "NFSPROC_READDIR ok read only first two directory entries") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_ok_read_only_first_directory_entry: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // read the first entry from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    int expected_number_of_entries = 2;
    char *expected_filenames[2] = {"..", "."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(rpc_connection_context, &fhandle, 0, 40, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_ok_read_directory_entries_in_batches, .description = "NFSPROC_READDIR ok read directory entries in batches") {
    RpcConnectionContext *rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_ok_read_directory_entries_in_batches: Failed to connect to the server\n");
    }

    Mount__FhStatus *fhstatus = mount_directory_success(rpc_connection_context, "/nfs_share");

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    int expected_number_of_entries = NFS_SHARE_NUMBER_OF_ENTRIES;
    char *expected_filenames[NFS_SHARE_NUMBER_OF_ENTRIES] = NFS_SHARE_ENTRIES;

    long posix_cookie = 0;  // start from beginning of the directory stream
    int eof = 0;
    int directory_entries_seen = 0;
    while(eof != 1 && directory_entries_seen < expected_number_of_entries) {
        Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
        cookie.value = posix_cookie;

        Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
        readdirargs.dir = &fhandle;
        readdirargs.cookie = &cookie;
        readdirargs.count = 30; // aim to read only one directory entry

        Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
        int status = nfs_procedure_16_read_from_directory(rpc_connection_context, readdirargs, readdirres);
        if(status != 0) {
            mount__fh_status__free_unpacked(fhstatus, NULL);

            free(readdirres);

            free_rpc_connection_context(rpc_connection_context);

            cr_fatal("NFSPROC_READDIR failed - status %d\n", status);
        }

        // validate ReadDirRes
        cr_assert_not_null(readdirres->nfs_status);
        cr_assert_eq(readdirres->nfs_status->stat, NFS__STAT__NFS_OK);
        cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
        cr_assert_not_null(readdirres->readdirok);

        cr_assert_not_null(readdirres->readdirok->entries);
        eof = readdirres->readdirok->eof;

        Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
        Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
        while(directory_entries_head != NULL && directory_entries_seen < expected_number_of_entries) {
            cr_assert_not_null(directory_entries_head->name);
            cr_assert_not_null(directory_entries_head->name->filename);

            // check this filename is in the 'expected_filenames' and we have not seen it already
            char *filename = directory_entries_head->name->filename;
            int filename_found = 0;
            for(int j = 0; j < expected_number_of_entries; j++) {
                // skip already seen filenames
                if(expected_filenames[j] == NULL) {
                    continue;
                }

                if(strcmp(filename, expected_filenames[j]) == 0) {
                    // mark the filename as seen
                    expected_filenames[j] = NULL;

                    filename_found = 1;

                    break;
                }
            }

            cr_assert(filename_found == 1, "Entry '%s' in procedure results is not among expected directory entries", filename);

            cr_assert_not_null(directory_entries_head->cookie);

            // update the cookie so that the next NFSPROC_READDIR call starts reading directories from where the previous call left off
            posix_cookie = directory_entries_head->cookie->value;

            if(directory_entries_seen == expected_number_of_entries) {
                cr_assert_null(directory_entries_head->nextentry);
            }

            directory_entries_head = directory_entries_head->nextentry;
            directory_entries_seen++;
        }

        nfs__read_dir_res__free_unpacked(readdirres, NULL);
    }

    cr_assert_eq(eof, 1);

    mount__fh_status__free_unpacked(fhstatus, NULL);

    free_rpc_connection_context(rpc_connection_context);
}

/*
* Permission tests
*/

Test(nfs_readdir_test_suite, readdir_no_read_permission, .description = "NFSPROC_READDIR no read permission") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_read directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_read_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_read", NFS__FTYPE__NFDIR);

    // now try to read entries in this directory /nfs_share/permission_test/only_owner_read, without having execute permission on it
    Nfs__FHandle only_owner_read_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_read_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_read_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_read_dir_diropres, NULL);
    only_owner_read_fhandle.nfs_filehandle = &only_owner_read_nfs_filehandle_copy;

    uint32_t gids[1] = {NON_DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *non_owner_credential = create_auth_sys_opaque_auth("test", NON_DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(non_owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_no_read_permission: Failed to connect to the server\n");
    }

    // fail since you don't have execute permission
    read_from_directory_fail(rpc_connection_context, &only_owner_read_fhandle, 0, 1000, NFS__STAT__NFSERR_ACCES);

    free_rpc_connection_context(rpc_connection_context);
}

Test(nfs_readdir_test_suite, readdir_has_read_permission_on_containing_directory, .description = "NFSPROC_READDIR has read permission on containing directory") {
    Mount__FhStatus *fhstatus = mount_directory_success(NULL, "/nfs_share");

    // lookup the permission_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *permission_test_dir_diropres = lookup_file_or_directory_success(NULL, &fhandle, "permission_test", NFS__FTYPE__NFDIR);

    // lookup the only_owner_read directory inside the permission_test directory
    Nfs__FHandle permission_test_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle permission_test_nfs_filehandle_copy = deep_copy_nfs_filehandle(permission_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(permission_test_dir_diropres, NULL);
    permission_test_fhandle.nfs_filehandle = &permission_test_nfs_filehandle_copy;

    Nfs__DirOpRes *only_owner_read_dir_diropres = lookup_file_or_directory_success(NULL, &permission_test_fhandle, "only_owner_read", NFS__FTYPE__NFDIR);

    // now try to read entries in this directory /nfs_share/permission_test/only_owner_read, without having execute permission on it
    Nfs__FHandle only_owner_read_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle only_owner_read_nfs_filehandle_copy = deep_copy_nfs_filehandle(only_owner_read_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(only_owner_read_dir_diropres, NULL);
    only_owner_read_fhandle.nfs_filehandle = &only_owner_read_nfs_filehandle_copy;

    uint32_t gids[1] = {DOCKER_IMAGE_TESTUSER_UID};
    Rpc__OpaqueAuth *owner_credential = create_auth_sys_opaque_auth("test", DOCKER_IMAGE_TESTUSER_UID, DOCKER_IMAGE_TESTUSER_GID, 1, gids);
    Rpc__OpaqueAuth *verifier = create_auth_none_opaque_auth();
    RpcConnectionContext *rpc_connection_context = create_rpc_connection_context_with_test_ipaddr_and_port(owner_credential, verifier, TEST_TRANSPORT_PROTOCOL);
    if(rpc_connection_context == NULL) {
        cr_fatal("readdir_has_read_permission_on_containing_directory: Failed to connect to the server\n");
    }

    int expected_number_of_entries = 4;
    char *expected_entries[] = {".", "..", "dir", "file.txt"};

    // succeed since you have write permission on containing directory
    Nfs__ReadDirRes *readdires = read_from_directory_success(rpc_connection_context, &only_owner_read_fhandle, 0, 1000, expected_number_of_entries, expected_entries);
    nfs__read_dir_res__free_unpacked(readdires, NULL);

    free_rpc_connection_context(rpc_connection_context);
}