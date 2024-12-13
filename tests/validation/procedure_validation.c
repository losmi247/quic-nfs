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
        cr_fatal("MOUNTPROC_MNT failed - status %d\n", status);
    }

    cr_assert_not_null(fhstatus);

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

    cr_assert_not_null(fhstatus->mnt_status);
    cr_assert_eq(fhstatus->mnt_status->stat, MOUNT__STAT__MNT_OK);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);

    cr_assert_not_null(fhstatus->directory);
    cr_assert_not_null(fhstatus->directory->nfs_filehandle);
    // it's hard to validate the nfs filehandle at client, so we don't do it

    return fhstatus;
}

/*
* Mounts the directory given by absolute path.
*
* The procedure results are validated assuming a non-MOUNT__STAT__MNT_OK Mount status, given in argument 'non_mnt_ok_status'.
*/
void mount_directory_fail(char *directory_absolute_path, Mount__Stat non_mnt_ok_status) {
    Mount__FhStatus *fhstatus = mount_directory(directory_absolute_path);

    cr_assert_not_null(fhstatus->mnt_status);
    cr_assert_eq(fhstatus->mnt_status->stat, non_mnt_ok_status);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DEFAULT_CASE);
    cr_assert_not_null(fhstatus->default_case);

    mount__fh_status__free_unpacked(fhstatus, NULL);
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_GETATTR to get its attributes.
* 
* Returns the Nfs__AttrStat returned by GETATTR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *get_attributes(Nfs__FHandle file_fhandle) {
    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(file_fhandle, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fatal("NFSPROC_GETATTR failed - status %d\n", status);
    }

    cr_assert_not_null(attrstat);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_GETATTR to get its attributes.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file has type given in 'expected_ftype'.
*
* Returns the Nfs__AttrStat returned by GETATTR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *get_attributes_success(Nfs__FHandle file_fhandle, Nfs__FType expected_ftype) {
    Nfs__AttrStat *attrstat = get_attributes(file_fhandle);

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);

    // validate attributes
    cr_assert_not_null(attrstat->attributes);
    validate_fattr(attrstat->attributes, NFS__FTYPE__NFDIR);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_GETATTR to get its attributes.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void get_attributes_fail(Nfs__FHandle file_fhandle, Nfs__Stat non_nfs_ok_status) {
    Nfs__AttrStat *attrstat = get_attributes(file_fhandle);

    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_SETATTR to update the attributes of
* the given file to the values given in the sattr argument.
* 
* Returns the Nfs__AttrStat returned by SETATTR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *set_attributes(Nfs__FHandle *file_fhandle, Nfs__SAttr *sattr) {
    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = file_fhandle;
    sattrargs.attributes = sattr;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(sattrargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fatal("NFSPROC_SETATTR failed - status %d\n", status);
    }

    cr_assert_not_null(attrstat);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_SETATTR to update the attributes of
* the given file to the values given in 'mode', 'uid', 'gid,' 'size', 'atime', 'mtime' arguments.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The new file attributes received in procedure results are validated assuming the file has type given in 'ftype'.
* 
* Returns the Nfs__AttrStat returned by SETATTR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *set_attributes_success(Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    Nfs__AttrStat *attrstat = set_attributes(file_fhandle, &sattr);

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);
    
    // validate attributes
    cr_assert_not_null(attrstat->attributes);
    Nfs__FAttr *fattr = attrstat->attributes;
    validate_fattr(attrstat->attributes, ftype);
    check_fattr_update(fattr, &sattr);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_SETATTR to update the attributes of
* the given file to the values given in 'mode', 'uid', 'gid,' 'size', 'atime', 'mtime' arguments.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void set_attributes_fail(Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    Nfs__AttrStat *attrstat = set_attributes(file_fhandle, &sattr);

    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LOOKUP to lookup the given filename
* inside the given directory.
* 
* Returns the Nfs__DirOpRes returned by LOOKUP procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *lookup_file_or_directory(Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(diropargs, diropres);
    if(status != 0) {
        free(diropres);
        cr_fatal("NFSPROC_LOOKUP failed - status %d\n", status);
    }

    cr_assert_not_null(diropres);

    return diropres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LOOKUP to lookup the given filename
* inside the given directory.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file has type given in 'expected_ftype'.
* 
* Returns the Nfs__DirOpRes returned by LOOKUP procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *lookup_file_or_directory_success(Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype) {
    Nfs__DirOpRes *diropres = lookup_file_or_directory(directory_fhandle, filename);

    // validate DirOpRes
    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK);
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
* Given the Nfs__FHandle of a directory, calls NFSPROC_LOOKUP to lookup the given filename
* inside the given directory.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void lookup_file_or_directory_fail(Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    Nfs__DirOpRes *diropres = lookup_file_or_directory(directory_fhandle, filename);

    // validate DirOpRes
    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
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
Nfs__ReadRes *read_from_file(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count) {
    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = file_fhandle;
    readargs.offset = offset;
    readargs.count = byte_count;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    int status = nfs_procedure_6_read_from_file(readargs, readres);
    if(status != 0) {
        free(readres);
        cr_fatal("NFSPROC_READ failed - status %d\n", status);
    }

    cr_assert_not_null(readres);

    return readres;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_READ to read up to 'byte_count' from 'offset' in the
* specified file.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file attributes before the read
* were 'attributes_before_read', and the read content is supposed to be 'expected_read_content' of size
* 'expected_read_size' bytes.
* Note that we can read from many file types - regular files, symbolic links, or character/block-special devices.
* 
* Returns the Nfs__ReadRes returned by READ procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_res__free_unpacked()'
* with the obtained ReadRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadRes
* and always call 'nfs__read_res__free_unpacked()' on it at some point.
*/
Nfs__ReadRes *read_from_file_success(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read, uint32_t expected_read_size, uint8_t *expected_read_content) {
    Nfs__ReadRes *readres = read_from_file(file_fhandle, offset, byte_count);

    // validate ReadRes
    cr_assert_not_null(readres->nfs_status);
    cr_assert_eq(readres->nfs_status->stat, NFS__STAT__NFS_OK);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_READRESBODY);
    cr_assert_not_null(readres->readresbody);

    // validate attributes
    cr_assert_not_null(readres->readresbody->attributes);
    Nfs__FAttr *read_fattr = readres->readresbody->attributes;
    validate_fattr(read_fattr, attributes_before_read->nfs_ftype->ftype);
    check_equal_fattr(attributes_before_read, read_fattr);
    // this is a famous problem - atime is going to be flushed to disk by the kernel only after 24hrs, we can't synchronously update it
    cr_assert(get_time(attributes_before_read->atime) <= get_time(read_fattr->atime));   // file was accessed
    cr_assert(get_time(attributes_before_read->mtime) == get_time(read_fattr->mtime));   // file was not modified
    cr_assert(get_time(attributes_before_read->ctime) == get_time(read_fattr->ctime));

    // validate read content
    cr_assert_not_null(readres->readresbody->nfsdata.data);
    ProtobufCBinaryData read_content = readres->readresbody->nfsdata;
    cr_assert_leq(read_content.len, byte_count);
    cr_assert_eq(read_content.len, expected_read_size, "Expected to read %d bytes but read %ld bytes", expected_read_size, read_content.len);
    
    cr_assert(memcmp(read_content.data, expected_read_content, expected_read_size) == 0); // compare the buffer we read and the expected buffer

    return readres;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_READ to read up to 'byte_count' from 'offset' in the
* specified file.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void read_from_file_fail(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__Stat non_nfs_ok_status) {
    Nfs__ReadRes *readres = read_from_file(file_fhandle, offset, byte_count);

    // validate ReadRes
    cr_assert_not_null(readres->nfs_status);
    cr_assert_eq(readres->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readres->default_case);

    nfs__read_res__free_unpacked(readres, NULL);
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_WRITE to write the given 'byte_count' at 'offset' in
* that file, from the specified source buffer. 
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

    writeargs.nfsdata.data = source_buffer;
    writeargs.nfsdata.len = byte_count;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(writeargs, attrstat);
    if(status != 0) {
        free(attrstat);
        cr_fatal("NFSPROC_WRITE failed - status %d\n", status);
    }

    cr_assert_not_null(attrstat);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_WRITE to write the given 'byte_count' at 'offset' in
* that file, from the specified source buffer.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file has type given in 'ftype'.
* 
* Returns the Nfs__AttrStat returned by WRITE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__attr_stat__free_unpacked()'
* with the obtained AttrStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__AttrStat
* and always call 'nfs__attr_stat__free_unpacked()' on it at some point.
*/
Nfs__AttrStat *write_to_file_success(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__FType ftype) {
    Nfs__AttrStat *attrstat = write_to_file(file_fhandle, offset, byte_count, source_buffer);

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);

    // validate attributes
    cr_assert_not_null(attrstat->attributes);
    Nfs__FAttr *fattr = attrstat->attributes;
    validate_fattr(fattr, ftype);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file, calls NFSPROC_WRITE to write the given 'byte_count' at 'offset' in
* that file, from the specified source buffer.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void write_to_file_fail(Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__Stat non_nfs_ok_status) {
    Nfs__AttrStat *attrstat = write_to_file(file_fhandle, offset, byte_count, source_buffer);

    cr_assert_not_null(attrstat->nfs_status);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_DEFAULT_CASE);
    cr_assert_not_null(attrstat->default_case);

    nfs__attr_stat__free_unpacked(attrstat, NULL);
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
        cr_fatal("NFSPROC_CREATE failed - status %d\n", status);
    }

    cr_assert_not_null(diropres);

    return diropres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_CREATE to create a file with the given filename
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid, size,
* atime, mtime arguments.
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

    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK);
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
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid, size,
* atime, mtime arguments.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
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

    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_REMOVE to delete the file with the given filename
* inside the given directory.
* 
* Returns the Nfs__NfsStat returned by REMOVE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *remove_file(Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_10_remove_file(diropargs, nfsstat);
    if(status != 0) {
        free(nfsstat);
        cr_fatal("NFSPROC_REMOVE failed - status %d\n", status);
    }

    cr_assert_not_null(nfsstat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_REMOVE to delete the file with the given filename
* inside the given directory.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__NfsStat returned by REMOVE procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *remove_file_success(Nfs__FHandle *directory_fhandle, char *filename) { 
    Nfs__NfsStat *nfsstat = remove_file(directory_fhandle, filename);

    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_REMOVE to delete the file with the given filename
* inside the given directory.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void remove_file_fail(Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    Nfs__NfsStat *nfsstat = remove_file(directory_fhandle, filename);

    cr_assert_eq(nfsstat->stat, non_nfs_ok_status);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

/*
* Given the Nfs__FHandle's of 'from' and 'to' directories, and Nfs__FileName's of 'from' and 'to' files/directories
* inside the 'from' and 'to' directores, calls NFSPROC_RENAME to rename the file with the given 'from' filename in the
* 'from' directory to the 'to' directory with the 'to' filename.
* 
* Returns the Nfs__NfsStat returned by RENAME procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *rename_file(Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename) {
    Nfs__FileName from_file_name = NFS__FILE_NAME__INIT;
    from_file_name.filename = from_filename;

    Nfs__DirOpArgs from = NFS__DIR_OP_ARGS__INIT;
    from.dir = from_directory_fhandle;
    from.name = &from_file_name;

    Nfs__FileName to_file_name = NFS__FILE_NAME__INIT;
    to_file_name.filename = to_filename;

    Nfs__DirOpArgs to = NFS__DIR_OP_ARGS__INIT;
    to.dir = to_directory_fhandle;
    to.name = &to_file_name;

    Nfs__RenameArgs renameargs = NFS__RENAME_ARGS__INIT;
    renameargs.from = &from;
    renameargs.to = &to;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_11_rename_file(renameargs, nfsstat);
    if(status != 0) {
        free(nfsstat);
        cr_fatal("NFSPROC_RENAME failed - status %d\n", status);
    }

    cr_assert_not_null(nfsstat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle's of 'from' and 'to' directories, and Nfs__FileName's of 'from' and 'to' files/directories
* inside the 'from' and 'to' directores, calls NFSPROC_RENAME to rename the file with the given 'from' filename in the
* 'from' directory to the 'to' directory with the 'to' filename.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__NfsStat returned by RENAME procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *rename_file_success(Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename) {
    Nfs__NfsStat *nfsstat = rename_file(from_directory_fhandle, from_filename, to_directory_fhandle, to_filename);

    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK);

    return nfsstat;
}

/*
* Given the Nfs__FHandle's of 'from' and 'to' directories, and Nfs__FileName's of 'from' and 'to' files/directories
* inside the 'from' and 'to' directores, calls NFSPROC_RENAME to rename the file with the given 'from' filename in the
* 'from' directory to the 'to' directory with the 'to' filename.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void rename_file_fail(Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename, Nfs__Stat non_nfs_ok_status) {
    Nfs__NfsStat *nfsstat = rename_file(from_directory_fhandle, from_filename, to_directory_fhandle, to_filename);

    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_MKDIR to create a directory with the given filename
* inside the given directory. The directory is created with initial attributes specified in sattr argument.
* 
* Returns the Nfs__DirOpRes returned by MKDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *create_directory(Nfs__FHandle *directory_fhandle, char *filename, Nfs__SAttr *sattr) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;
    createargs.attributes = sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_14_create_directory(createargs, diropres);
    if(status != 0) {
        free(diropres);
        cr_fatal("NFSPROC_MKDIR failed - status %d\n", status);
    }

    cr_assert_not_null(diropres);

    return diropres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_MKDIR to create a directory with the given filename
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid, atime, 
* mtime arguments (size attribute not used, as it can not be set for directories).
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The file attributes received in procedure results are validated assuming the file has type given in 'ftype'.
* 
* Returns the Nfs__DirOpRes returned by MKDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__dir_op_res__free_unpacked()'
* with the obtained DirOpRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__DirOpRes
* and always call 'nfs__dir_op_res__free_unpacked()' on it at some point.
*/
Nfs__DirOpRes *create_directory_success(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) { 
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = -1;    // we do not (can not) set size attribute for directories
    sattr.atime = atime;
    sattr.mtime = mtime;
    
    Nfs__DirOpRes *diropres = create_directory(directory_fhandle, filename, &sattr);

    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK);
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
* Given the Nfs__FHandle of a directory, calls NFSPROC_MKDIR to create a directory with the given filename
* inside the given directory. The file is created with initial attributes specified in mode, uid, gid, atime, 
* mtime arguments (size attribute not used, as it can not be set for directories).
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void create_directory_fail(Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = -1;
    sattr.atime = atime;
    sattr.mtime = mtime;

    Nfs__DirOpRes *diropres = create_directory(directory_fhandle, filename, &sattr);

    cr_assert_not_null(diropres->nfs_status);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_RMDIR to delete the directory with the given 
* filename inside the given directory.
* 
* Returns the Nfs__NfsStat returned by RMDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *remove_directory(Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_15_remove_directory(diropargs, nfsstat);
    if(status != 0) {
        free(nfsstat);
        cr_fatal("NFSPROC_RMDIR failed - status %d\n", status);
    }

    cr_assert_not_null(nfsstat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_RMDIR to delete the directory with the given
* filename inside the given directory.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__NfsStat returned by RMDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *remove_directory_success(Nfs__FHandle *directory_fhandle, char *filename) { 
    Nfs__NfsStat *nfsstat = remove_directory(directory_fhandle, filename);

    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_RMDIR to delete the directory with the given 
* filename inside the given directory.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void remove_directory_fail(Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    Nfs__NfsStat *nfsstat = remove_directory(directory_fhandle, filename);

    cr_assert_eq(nfsstat->stat, non_nfs_ok_status);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_READDIR to read entries from this directory starting at
* offset specified by the Nfs 'cookie', with total size up to 'byte_count'.
* 
* Returns the Nfs__ReadDirRes returned by READDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_dir_res__free_unpacked()'
* with the obtained ReadDirRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadDirRes
* and always call 'nfs__read_dir_res__free_unpacked()' on it at some point.
*/
Nfs__ReadDirRes *read_from_directory(Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count) {
    Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
    nfs_cookie.value = cookie;

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = directory_fhandle;
    readdirargs.cookie = &nfs_cookie;
    readdirargs.count = byte_count;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(readdirargs, readdirres);
    if(status != 0) {
        free(readdirres);
        cr_fatal("NFSPROC_READDIR failed - status %d\n", status);
    }

    cr_assert_not_null(readdirres);

    return readdirres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_READDIR to read entries from this directory starting at
* offset specified by the Nfs 'cookie', with total size up to 'byte_count'.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* The filenames in the directory entries in the procedure results are checked to exactly be equal to the set of
* filenames given in 'expected_filenames' argument which consists of 'expected_number_of_entries' filenames.
* 
* Returns the Nfs__ReadDirRes returned by READDIR procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_dir_res__free_unpacked()'
* with the obtained ReadDirRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadDirRes
* and always call 'nfs__read_dir_res__free_unpacked()' on it at some point.
*/
Nfs__ReadDirRes *read_from_directory_success(Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, int expected_number_of_entries, char *expected_filenames[]) {
    Nfs__ReadDirRes *readdirres = read_from_directory(directory_fhandle, cookie, byte_count);

    // validate ReadDirRes
    cr_assert_not_null(readdirres->nfs_status);
    cr_assert_eq(readdirres->nfs_status->stat, NFS__STAT__NFS_OK);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_READDIROK);
    cr_assert_not_null(readdirres->readdirok);

    cr_assert_not_null(readdirres->readdirok->entries);
    //cr_assert_eq(readdirres->readdirok->eof, 1);    // we try and read all directory entries in this test

    Nfs__DirectoryEntriesList *directory_entries = readdirres->readdirok->entries;
    Nfs__DirectoryEntriesList *directory_entries_head = directory_entries;
    for(int i = 0; i < expected_number_of_entries; i++) {
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

        if(i < expected_number_of_entries - 1) {
            cr_assert_not_null(directory_entries_head->nextentry);
        }
        else{
            cr_assert_null(directory_entries_head->nextentry);
        }

        directory_entries_head = directory_entries_head->nextentry;
    }

    return readdirres;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_READDIR to read entries from this directory starting at
* offset specified by the Nfs 'cookie', with total size up to 'byte_count'.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void read_from_directory_fail(Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, Nfs__Stat non_nfs_ok_status) {
    Nfs__ReadDirRes *readdirres = read_from_directory(directory_fhandle, cookie, byte_count);

    // validate ReadDirRes
    cr_assert_not_null(readdirres->nfs_status);
    cr_assert_eq(readdirres->nfs_status->stat, non_nfs_ok_status);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}