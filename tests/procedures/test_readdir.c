#include "tests/test_common.h"

#include <stdlib.h>
#include <stdio.h>

/*
* NFSPROC_READDIR (16) tests
*/

Test(nfs_test_suite, readdir_ok, .description = "NFSPROC_READDIR ok") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    int expected_number_of_entries = 5;
    char *expected_file_names[5] = {"..", ".", "dir1", "a.txt", "test_file.txt"};

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 1);    // we try and read all directory entries in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    for(int i = 0; i < expected_number_of_entries; i++) {
        cr_assert_not_null(directory_entries_head->name);
        cr_assert_str_eq(directory_entries_head->name->filename, expected_file_names[i], "Expected '%s' but found '%s'", expected_file_names[i], directory_entries_head->name->filename);
        cr_assert_not_null(directory_entries_head->cookie);

        if(i < expected_number_of_entries - 1) {
            cr_assert_not_null(directory_entries_head->nextentry);
        }
        else{
            cr_assert_null(directory_entries_head->nextentry);
        }

        directory_entries_head = directory_entries_head->nextentry;
    }

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_no_such_directory, .description = "NFSPROC_READDIR no such directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // try to read from a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = 12345678912345;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFSERR_NOENT);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_not_a_directory, .description = "NFSPROC_READDIR not a directory") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle root_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    root_fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&root_fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to readdir this test_file.txt (non-directory)
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &file_fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFSERR_NOTDIR);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

// NFSPROC_READDIR (16)
Test(nfs_test_suite, readdir_ok_lookup_then_readdir, .description = "NFSPROC_READDIR ok lookup then readdir") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // lookup directory dir1 inside the mounted directory
    Nfs__FHandle root_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    root_fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory(&root_fhandle, "dir1", NFS__FTYPE__NFDIR);

    // try to readdir this directory dir1
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &dir_fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 1000;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    // validate ReadDirRes
    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 1);    // we try and read all directory entries in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, "..", "Expected '..' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_not_null(directory_entries_head->nextentry);

    directory_entries_head = directory_entries_head->nextentry;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, ".", "Expected '.' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_null(directory_entries_head->nextentry);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_only_first_directory_entry, .description = "NFSPROC_READDIR ok read only first directory entry") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
    cookie.value = 0; // start from beginning of the directory stream

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = &fhandle;
    readdirargs.cookie = &cookie;
    readdirargs.count = 20; // read only one directory entry

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        mount__fh_status__free_unpacked(fhstatus, NULL);

        free(readdirres);

        cr_fail("NFSPROC_READDIR failed - status %d\n", status);
    }

    // validate ReadDirRes
    cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    cr_assert_eq(readdirres->readdirok->eof, 0);    // we try and read only the first directory entry in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    cr_assert_not_null(directory_entries_head->name);
    cr_assert_str_eq(directory_entries_head->name->filename, "..", "Expected '..' but found '%s'", directory_entries_head->name->filename);
    cr_assert_not_null(directory_entries_head->cookie);
    cr_assert_null(directory_entries_head->nextentry);

    mount__fh_status__free_unpacked(fhstatus, NULL);
    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_directory_entries_in_batches, .description = "NFSPROC_READDIR ok read directory entries in batches") {
    Mount__FhStatus *fhstatus = mount_directory("/nfs_share");

    // read from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = fhstatus->directory->nfs_filehandle;

    int expected_number_of_entries = 5;
    char *expected_file_names[5] = {"..", ".", "dir1", "a.txt", "test_file.txt"};

    long posix_cookie = 0;  // start from beginning of the directory stream
    int eof = 0;
    int index = 0;
    while(eof != 1 && index < expected_number_of_entries) {
        Nfs__NfsCookie cookie = NFS__NFS_COOKIE__INIT;
        cookie.value = posix_cookie;

        Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
        readdirargs.dir = &fhandle;
        readdirargs.cookie = &cookie;
        readdirargs.count = 30; // aim to read only one directory entry

        Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
        int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
        if(status != 0) {
            mount__fh_status__free_unpacked(fhstatus, NULL);

            free(readdirres);

            cr_fail("NFSPROC_READDIR failed - status %d\n", status);
        }

        // validate ReadDirRes
        cr_assert_eq(readdirres->status, NFS__STAT__NFS_OK);
        cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
        cr_assert_not_null(readdirres->readdirok);

        cr_assert_not_null(readdirres->readdirok->entries);
        eof = readdirres->readdirok->eof;

        Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
        Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
        while(directory_entries_head != NULL && index < expected_number_of_entries) {
            cr_assert_not_null(directory_entries_head->name);
            cr_assert_str_eq(directory_entries_head->name->filename, expected_file_names[index], "Expected '%s' but found '%s'", expected_file_names[index], directory_entries_head->name->filename);
            cr_assert_not_null(directory_entries_head->cookie);

            // update the cookie so that the next NFSPROC_READDIR call starts reading directories from where the previous call left off
            posix_cookie = directory_entries_head->cookie->value;

            if(index == expected_number_of_entries-1) {
                cr_assert_null(directory_entries_head->nextentry);
            }

            directory_entries_head = directory_entries_head->nextentry;
            index++;
        }

        nfs__read_dir_res__free_unpacked(readdirres, NULL);
    }

    cr_assert_eq(eof, 1);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}