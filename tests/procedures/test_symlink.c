#include "tests/test_common.h"

#include <stdio.h>
#include <time.h>

/*
* NFSPROC_SYMLINK (13) tests
*/

TestSuite(nfs_symlink_test_suite);

Test(nfs_symlink_test_suite, symlink_ok, .description = "NFSPROC_SYMLINK ok") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "symlink_test", NFS__FTYPE__NFDIR);

    // create a symbolic link 'symlink' to /nfs_share/symlink_test/existing_file, inside this /nfs_share/symlink_test directory
    Nfs__FHandle symlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_test_dir_diropres, NULL);
    symlink_test_dir_fhandle.nfs_filehandle = &symlink_test_dir_nfs_filehandle_copy;

    Nfs__Path target = NFS__PATH__INIT;
    target.path = "/nfs_share/symlink_test/existing_file";
    Nfs__NfsStat *nfsstat = create_symbolic_link_success(&symlink_test_dir_fhandle, "symlink", &target);
    nfs__nfs_stat__free_unpacked(nfsstat, NULL);

    // lookup the created 'symlink' file to verify its file type is NFLNK
    Nfs__DirOpRes *symlink_diropres = lookup_file_or_directory_success(&symlink_test_dir_fhandle, "symlink", NFS__FTYPE__NFLNK);
    nfs__dir_op_res__free_unpacked(symlink_diropres, NULL);
}

Test(nfs_symlink_test_suite, symlink_no_such_directory, .description = "NFSPROC_SYMLINK no such directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // try to create a symbolic link 'symlink' (to some target) inside a nonexistent directory
    NfsFh__NfsFileHandle nfs_filehandle = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle.inode_number = NONEXISTENT_INODE_NUMBER;
    nfs_filehandle.timestamp = time(NULL);

    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    fhandle.nfs_filehandle = &nfs_filehandle;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";

    create_symbolic_link_fail(&fhandle, "symlink", &path, NFS__STAT__NFSERR_NOENT);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

Test(nfs_symlink_test_suite, symlink_create_a_symbolic_link_in_a_non_directory, .description = "NFSPROC_SYMLINK create symbolic link in a non-directory") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the test_file.txt inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *dir_diropres = lookup_file_or_directory_success(&fhandle, "test_file.txt", NFS__FTYPE__NFREG);

    // try to create a symbolic link "symlink" (to some target) inside this test_file.txt file
    Nfs__FHandle file_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle file_nfs_filehandle_copy = deep_copy_nfs_filehandle(dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(dir_diropres, NULL);
    file_fhandle.nfs_filehandle = &file_nfs_filehandle_copy;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";

    create_symbolic_link_fail(&file_fhandle, "symlink", &path, NFS__STAT__NFSERR_NOTDIR);
}

Test(nfs_symlink_test_suite, symlink_file_name_too_long, .description = "NFSPROC_SYMLINK file name too long") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "symlink_test", NFS__FTYPE__NFDIR);

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

    create_symbolic_link_fail(&symlink_test_dir_fhandle, filename, &path, NFS__STAT__NFSERR_NAMETOOLONG);
}

Test(nfs_symlink_test_suite, create_symbolic_link_that_is_an_already_existing_file, .description = "NFSPROC_SYMLINK create symbolic link that is an already existing file") {
    Mount__FhStatus *fhstatus = mount_directory_success("/nfs_share");

    // lookup the symlink_test directory inside the mounted directory
    Nfs__FHandle fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle nfs_filehandle_copy = deep_copy_nfs_filehandle(fhstatus->directory->nfs_filehandle);
    mount__fh_status__free_unpacked(fhstatus, NULL);
    fhandle.nfs_filehandle = &nfs_filehandle_copy;

    Nfs__DirOpRes *symlink_test_dir_diropres = lookup_file_or_directory_success(&fhandle, "symlink_test", NFS__FTYPE__NFDIR);

    // try to create a symbolic link (to some target) at /nfs_share/symlink_test/existing_file that already exists
    Nfs__FHandle symlink_test_dir_fhandle = NFS__FHANDLE__INIT;
    NfsFh__NfsFileHandle symlink_test_dir_nfs_filehandle_copy = deep_copy_nfs_filehandle(symlink_test_dir_diropres->diropok->file->nfs_filehandle);
    nfs__dir_op_res__free_unpacked(symlink_test_dir_diropres, NULL);
    symlink_test_dir_fhandle.nfs_filehandle = &symlink_test_dir_nfs_filehandle_copy;

    Nfs__Path path = NFS__PATH__INIT;
    path.path = "/nfs_share/symlink_test/existing_file";
    
    create_symbolic_link_fail(&symlink_test_dir_fhandle, "existing_file", &path, NFS__STAT__NFSERR_EXIST);
}