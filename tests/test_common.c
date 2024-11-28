#include "test_common.h"

/*
* Mounts the directory given by absolute path. Returns the Mount__FhStatus.
*
* The user of this function takes on the responsibility to call 'mount_fh_status_free_unpacked()'
* with the obtained FhStatus. 
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Mount__FhStatus
* and always call 'mount_fh_status_free_unpacked()' on it at some point.
*/
Mount__FhStatus *mount_directory(char *directory_absolute_path) {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = directory_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(dirpath, fhstatus);
    if (status != 0) {
        free(fhstatus);
        cr_fail("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_eq(fhstatus->status, 0);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);

    cr_assert_not_null(fhstatus->directory);
    cr_assert_not_null(fhstatus->directory->nfs_filehandle);
    // it's hard to validate the nfs filehandle at client, so we don't do it

    return fhstatus;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LOOKUP to lookup the given filename
* inside the given directory. Checks that the looked up file has the given 'expected_ftype' file type.
* 
* Returns the Nfs__DirOpRes.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__read_dir_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *lookup_file_or_directory(Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(diropargs, diropres);
    if(status != 0) {
        free(diropres);
        cr_fail("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    cr_assert_eq(diropres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DIROPOK);
    cr_assert_not_null(diropres->diropok);

    cr_assert_not_null(diropres->diropok->file);
    cr_assert_not_null(diropres->diropok->file->nfs_filehandle);     // can't validate NFS filehandle contents as a client
    validate_fattr(diropres->diropok->attributes, expected_ftype); // can't validate any other attributes

    return diropres;
}

/*
* Creates a deep copy of the given NfsFh__NfsFileHandle.
*/
NfsFh__NfsFileHandle deep_copy_nfs_filehandle(NfsFh__NfsFileHandle *nfs_filehandle) {
    NfsFh__NfsFileHandle nfs_filehandle_copy = NFS_FH__NFS_FILE_HANDLE__INIT;
    nfs_filehandle_copy.inode_number = nfs_filehandle->inode_number;
    nfs_filehandle_copy.timestamp = nfs_filehandle->timestamp;

    return nfs_filehandle_copy;
}

/*
* Validates FAttr fields that can be check for correctnes from the client.
*/
void validate_fattr(Nfs__FAttr *fattr, Nfs__FType ftype) {
    cr_assert_neq(fattr, NULL);

    cr_assert_eq(fattr->type, ftype);
    cr_assert_not_null(fattr->atime);
    cr_assert_not_null(fattr->mtime);
    cr_assert_not_null(fattr->ctime);

    // other fattr fields are difficult to validate
}

uint64_t get_time(Nfs__TimeVal *timeval) {
    uint64_t sec = timeval->seconds;
    uint64_t milisec = timeval->useconds;
    return sec * 1000 + milisec;
}

/*
* Checks two FAttr structures of same file for equality, excluding atime, mtime, ctime fields.
*/
void check_equal_fattr(Nfs__FAttr *fattr1, Nfs__FAttr *fattr2) {
    cr_assert_eq(fattr1->type, fattr2->type);
    cr_assert_eq(fattr1->mode, fattr2->mode);

    cr_assert_eq(fattr1->nlink, fattr2->nlink);
    cr_assert_eq(fattr1->uid, fattr2->uid);
    cr_assert_eq(fattr1->gid, fattr2->gid);

    cr_assert_eq(fattr1->size, fattr2->size);
    cr_assert_eq(fattr1->blocksize, fattr2->blocksize);
    cr_assert_eq(fattr1->rdev, fattr2->rdev);
    cr_assert_eq(fattr1->blocks, fattr2->blocks);
    cr_assert_eq(fattr1->fsid, fattr2->fsid);
    cr_assert_eq(fattr1->fileid, fattr2->fileid);
}