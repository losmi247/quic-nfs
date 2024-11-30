#include "procedure_validation.h"

uint64_t get_time(Nfs__TimeVal *timeval) {
    uint64_t sec = timeval->seconds;
    uint64_t milisec = timeval->useconds;
    return sec * 1000 + milisec;
}

/*
* Mounts the directory given by absolute path. Returns the Mount__FhStatus.
*
* Returns the Mount__FhStatus returned by MOUNT procedure.
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
* Returns the Nfs__DirOpRes returned by LOOKUP procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
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
* Given the Nfs__FHandle of a file, calls NFSPROC_READ to read up to 'byte_count' from 'offset' in the
* specified file.
* 
* Returns the Nfs__ReadRes returned by READ procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_res__free_unpacked()'
* with the obtained ReadRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadRes
* and always call 'nfs__read_res__free_unpacked()' on it at some point.
*/
Nfs__ReadRes *read_from_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read) {
    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = file_fhandle;
    readargs.count = byte_count;
    readargs.offset = offset;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    int status = nfs_procedure_6_read_from_file(readargs, readres);
    if(status != 0) {
        free(readres);
        cr_fail("NFSPROC_READ failed - status %d\n", status);
    }

    // validate ReadRes
    cr_assert_eq(readres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_READRESBODY);
    cr_assert_not_null(readres->readresbody);

    // validate attributes
    cr_assert_not_null(readres->readresbody->attributes);
    Nfs__FAttr *read_fattr = readres->readresbody->attributes;
    validate_fattr(read_fattr, NFS__FTYPE__NFREG);
    check_equal_fattr(attributes_before_read, read_fattr);
    // this is a famous problem - atime is going to be flushed to disk by the kernel only after 24hrs, we can't synchronously update it
    cr_assert(get_time(attributes_before_read->atime) <= get_time(read_fattr->atime));   // file was accessed
    cr_assert(get_time(attributes_before_read->mtime) == get_time(read_fattr->mtime));   // file was not modified
    cr_assert(get_time(attributes_before_read->ctime) == get_time(read_fattr->ctime));

    // validate read content
    cr_assert_not_null(readres->readresbody->nfsdata.data);
    ProtobufCBinaryData read_content = readres->readresbody->nfsdata;
    cr_assert_leq(read_content.len, byte_count); // reads up to 'byte_count' bytes

    return readres;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_WRITE to write the given 'byte_count' at 'offset' in
* that file, from the specified source buffer.
* inside the given directory. Checks that the looked up file has the given 'expected_ftype' file type.
* 
* Returns the Nfs__AttrStat returned by WRITE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *write_to_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer) {
    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = offset;
    writeargs.totalcount = 0;  // unused

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fail("NFSPROC_WRITE failed - status %d\n", status);
    }

    cr_assert_eq(attrstat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    validate_fattr(attrstat->attributes, NFS__FTYPE__NFDIR);

    return attrstat;
}