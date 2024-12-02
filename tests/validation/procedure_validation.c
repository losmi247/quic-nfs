#include "procedure_validation.h"

uint64_t get_time(Nfs__TimeVal *timeval) {
    uint64_t sec = timeval->seconds;
    uint64_t milisec = timeval->useconds;
    return sec * 1000 + milisec;
}

/*
* Mounts the directory given by absolute path.
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

    return fhstatus;
}

/*
* Mounts the directory given by absolute path.
*
* The procedure results are validated assuming MOUNT__STAT__MNT_OK Mount status.
*
* Returns the Mount__FhStatus returned by MOUNT procedure.
*
* The user of this function takes on the responsibility to call 'mount_fh_status_free_unpacked()'
* with the obtained FhStatus. 
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Mount__FhStatus
* and always call 'mount_fh_status_free_unpacked()' on it at some point.
*/
Mount__FhStatus *mount_directory_success(char *directory_absolute_path) {
    Mount__FhStatus *fhstatus = mount_directory(directory_absolute_path);

    cr_assert_eq(fhstatus->status, MOUNT__STAT__MNT_OK);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);

    cr_assert_not_null(fhstatus->directory);
    cr_assert_not_null(fhstatus->directory->nfs_filehandle);
    // it's hard to validate the nfs filehandle at client, so we don't do it

    return fhstatus;
}

/*
* Mounts the directory given by absolute path.
*
* The procedure results are validated assuming a non-MOUNT__STAT__MNT_OK Mount status, given in argument 'non-nfs-ok-status'.
*/
void mount_directory_fail(char *directory_absolute_path, Mount__Stat non_mnt_ok_status) {
    Mount__FhStatus *fhstatus = mount_directory(directory_absolute_path);

    cr_assert_eq(fhstatus->status, non_mnt_ok_status);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_SETATTR to update the attributes of
* the given file to the values given in 'mode', 'uid', 'gid,' 'size', 'atime', 'mtime' arguments.
* The new file attributes are validated assuming the file has type given in 'ftype'.
* 
* Returns the Nfs__AttrStat returned by SETATTR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *set_attributes(Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = file_fhandle;
    sattrargs.attributes = &sattr;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fail("NFSPROC_SETATTR failed - status %d\n", status);
    }

    // validate AttrStat
    cr_assert_eq(attrstat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    

    // validate attributes
    cr_assert_not_null(attrstat->attributes);
    Nfs__FAttr *fattr = attrstat->attributes;
    validate_fattr(attrstat->attributes, ftype);
    check_fattr_update(fattr, &sattr);

    return attrstat;
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

    // validate Nfs filehandle
    cr_assert_not_null(diropres->diropok->file);
    cr_assert_not_null(diropres->diropok->file->nfs_filehandle);

    // validate attributes
    cr_assert_not_null(diropres->diropok->attributes);
    Nfs__FAttr *fattr = diropres->diropok->attributes;
    validate_fattr(fattr, expected_ftype);

    return diropres;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_READ to read up to 'byte_count' from 'offset' in the
* specified file.
* The resulting AttrStat returned by READ procedure are validated using the 'ftype' provided as argument 
* (we can read from many file types - regular files, symbolic links, block-special devices, etc.).
* 
* Returns the Nfs__ReadRes returned by READ procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_res__free_unpacked()'
* with the obtained ReadRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadRes
* and always call 'nfs__read_res__free_unpacked()' on it at some point.
*/
Nfs__ReadRes *read_from_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read, Nfs__FType ftype) {
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
    validate_fattr(read_fattr, ftype);
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
* The resulting AttrStat returned by WRITE procedure are validated using the 'ftype' provided as argument 
* (we can write to many file types - regular files, symbolic links, block-special devices, etc.).
* 
* Returns the Nfs__AttrStat returned by WRITE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *write_to_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__FType ftype) {
    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = offset;
    writeargs.totalcount = 0;  // unused

    writeargs.nfsdata.data = source_buffer;
    writeargs.nfsdata.len = byte_count;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fail("NFSPROC_WRITE failed - status %d\n", status);
    }

    cr_assert_eq(attrstat->status, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);

    cr_assert_not_null(attrstat->attributes);
    Nfs__FAttr *fattr = attrstat->attributes;
    validate_fattr(fattr, ftype);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_CREATE to create a file with the given filename
* inside the given directory. The file is created with initial attributes specified in sattr argument.
* 
* Returns the Nfs__DirOpRes returned by CREATE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *create_file(Nfs__FHandle *directory_fhandle, char *filename, Nfs__SAttr *sattr) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;
    createargs.attributes = sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_9_create_file(createargs, diropres);
    if(status != 0) {
        free(diropres);
        cr_fail("NFSPROC_CREATE failed - status %d\n", status);
    }

    return diropres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_CREATE to create a file with the given filename
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid
* arguments.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file has type given in 'ftype'.
* 
* Returns the Nfs__DirOpRes returned by CREATE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *create_file_success(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) { 
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;
    
    Nfs__DirOpRes *diropres = create_file(directory_fhandle, filename, &sattr);

    cr_assert_eq(diropres->status, NFS__STAT__NFS_OK);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DIROPOK);
    cr_assert_not_null(diropres->diropok);

    // validate Nfs filehandle
    cr_assert_not_null(diropres->diropok->file);
    cr_assert_not_null(diropres->diropok->file->nfs_filehandle);

    // validate attributes
    cr_assert_not_null(diropres->diropok->attributes);
    Nfs__FAttr *fattr = diropres->diropok->attributes;
    validate_fattr(fattr, ftype);
    check_fattr_update(fattr, &sattr);

    return diropres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_CREATE to create a file with the given filename
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid
* arguments.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non-nfs-ok-status'.
* The file attributes received in procedure results are validated assuming the file has type given in 'ftype'.
*/
void create_file_fail(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    Nfs__DirOpRes *diropres = create_file(directory_fhandle, filename, &sattr);

    cr_assert_eq(diropres->status, non_nfs_ok_status);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}