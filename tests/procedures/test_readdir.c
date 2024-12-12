#include "tests/test_common.h"

#include <stdlib.h>
#include <stdio.h>

/*
* NFSPROC_READDIR (16) tests
*/

Test(nfs_test_suite, readdir_ok, .description = "NFSPROC_READDIR ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // read all entries from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    int expected_number_of_entries = NFS_SHARE_NUMBER_OF_ENTRIES;
    char *expected_filenames[NFS_SHARE_NUMBER_OF_ENTRIES] = NFS_SHARE_ENTRIES;

    Nfs__ReadDirRes *readdirres = read_from_directory_success(&fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_no_such_directory, .description = "NFSPROC_READDIR no such directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to read from a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    read_from_directory_fail(&fhandle, 0, 1000, NFS__STAT__NFSERR_NOENT);  // cookie=0, start from beginning of the directory stream

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_test_suite, readdir_not_a_directory, .description = "NFSPROC_READDIR not a directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to readdir this test_file.txt (non-directory)
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    read_from_directory_fail(&file_fhandle, 0, 1000, NFS__STAT__NFSERR_NOTDIR);  // cookie=0, start from beginning of the directory stream
}

Test(nfs_test_suite, readdir_ok_lookup_then_readdir, .description = "NFSPROC_READDIR ok lookup then readdir") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup directory write_test inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *diropres = lookup_file_or_directory_success(&fhandle, "write_test", NFS__FTYPE__NFDIR);

    // read all entries in this directory write_test
    Nfs__FHandle dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(diropres, NULL);
    dir_fhandle.nfs_filehandle = &dir_nfs_filehandle_copy;

    int expected_number_of_entries = 3;
    char *expected_filenames[5] = {"..", ".", "write_test_file.txt"};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(&dir_fhandle, 0, 1000, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_only_first_directory_entry, .description = "NFSPROC_READDIR ok read only first directory entry") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // read the first entry from the /nfs_share directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    int expected_number_of_entries = 1;
    char *expected_filenames[1] = {".."};

    Nfs__ReadDirRes *readdirres = read_from_directory_success(&fhandle, 0, 20, expected_number_of_entries, expected_filenames);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

Test(nfs_test_suite, readdir_ok_read_directory_entries_in_batches, .description = "NFSPROC_READDIR ok read directory entries in batches") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

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
        int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
        if(status != 0) {
            mount__fh_status__free_unpacked(fhstatus, NULL);

            free(readdirres);

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
}