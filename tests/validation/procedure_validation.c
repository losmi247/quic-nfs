#include "procedure_validation.h"

/*
* Returns -1, 0, 1 depending on whether the first time is less than, equal to, or
* greater than the second time.
*/
int compare_time(Nfs__TimeVal *timeval1, Nfs__TimeVal *timeval2) {
    if(timeval1->seconds < timeval2->seconds) {
        return -1;
    }

    if(timeval2->seconds < timeval1->seconds) {
        return 1;
    }

    if(timeval1->useconds == timeval2->useconds) {
        return 0;
    }

    return timeval1->useconds < timeval2->useconds ? -1 : 1;
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
Mount__FhStatus *mount_directory(RpcConnectionContext *rpc_connection_context, char *directory_absolute_path) {
    Mount__DirPath dirpath = MOUNT__DIR_PATH__INIT;
    dirpath.path = directory_absolute_path;

    Mount__FhStatus *fhstatus = malloc(sizeof(Mount__FhStatus));
    int status = mount_procedure_1_add_mount_entry(rpc_connection_context, dirpath, fhstatus);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Mount__FhStatus *mount_directory_success(RpcConnectionContext *rpc_connection_context, char *directory_absolute_path) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Mount__FhStatus *fhstatus = mount_directory(rpc_connection_context, directory_absolute_path);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(fhstatus->mnt_status);
    char *expected_mnt_stat = mount_stat_to_string(MOUNT__STAT__MNT_OK), *found_mount_stat = mount_stat_to_string(fhstatus->mnt_status->stat);
    cr_assert_eq(fhstatus->mnt_status->stat, MOUNT__STAT__MNT_OK, "Expected NfsStat %s but got %s", expected_mnt_stat, found_mount_stat);
    free(expected_mnt_stat);
    free(found_mount_stat);
    cr_assert_eq(fhstatus->fhstatus_body_case, MOUNT__FH_STATUS__FHSTATUS_BODY_DIRECTORY);

    cr_assert_not_null(fhstatus->directory);
    cr_assert_not_null(fhstatus->directory->nfs_filehandle);
    // it's hard to validate the nfs filehandle at client, so we don't do it

    return fhstatus;
}

/*
* Mounts the directory given by absolute path, by calling the MOUNTPROC_MNT procedure.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-MOUNT__STAT__MNT_OK Mount status, given in argument 'non_mnt_ok_status'.
*/
void mount_directory_fail(RpcConnectionContext *rpc_connection_context, char *directory_absolute_path, Mount__Stat non_mnt_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Mount__FhStatus *fhstatus = mount_directory(rpc_connection_context, directory_absolute_path);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(fhstatus->mnt_status);
    char *expected_mnt_stat = mount_stat_to_string(non_mnt_ok_status), *found_mount_stat = mount_stat_to_string(fhstatus->mnt_status->stat);
    cr_assert_eq(fhstatus->mnt_status->stat, non_mnt_ok_status, "Expected NfsStat %s but got %s", expected_mnt_stat, found_mount_stat);
    free(expected_mnt_stat);
    free(found_mount_stat);
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
Nfs__AttrStat *get_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle file_fhandle) {
    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_1_get_file_attributes(rpc_connection_context, file_fhandle, attrstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__AttrStat *get_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle file_fhandle, Nfs__FType expected_ftype) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = get_attributes(rpc_connection_context, file_fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(attrstat->body_case, NFS__ATTR_STAT__BODY_ATTRIBUTES);

    // validate attributes
    cr_assert_not_null(attrstat->attributes);
    validate_fattr(attrstat->attributes, NFS__FTYPE__NFDIR);

    return attrstat;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_GETATTR to get its attributes.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void get_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle file_fhandle, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = get_attributes(rpc_connection_context, file_fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__AttrStat *set_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, Nfs__SAttr *sattr) {
    Nfs__SAttrArgs sattrargs = NFS__SATTR_ARGS__INIT;
    sattrargs.file = file_fhandle;
    sattrargs.attributes = sattr;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_2_set_file_attributes(rpc_connection_context, sattrargs, attrstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__AttrStat *set_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = set_attributes(rpc_connection_context, file_fhandle, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void set_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = set_attributes(rpc_connection_context, file_fhandle, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__DirOpRes *lookup_file_or_directory(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_4_look_up_file_name(rpc_connection_context, diropargs, diropres);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__DirOpRes *lookup_file_or_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__FType expected_ftype) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = lookup_file_or_directory(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate DirOpRes
    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void lookup_file_or_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = lookup_file_or_directory(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate DirOpRes
    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(diropres->body_case, NFS__DIR_OP_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(diropres->default_case);

    nfs__dir_op_res__free_unpacked(diropres, NULL);
}

/*
* Given the Nfs__FHandle of a symbolic link, calls NFSPROC_READLINK to read the pathname contained inside that
* symbolic link.
* 
* Returns the Nfs__ReadLinkRes returned by READLINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_link_res__free_unpacked()'
* with the obtained ReadLinkRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadLinkRes
* and always call 'nfs__read_link_res__free_unpacked()' on it at some point.
*/
Nfs__ReadLinkRes *read_from_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *symlink_fhandle) {
    Nfs__ReadLinkRes *readlinkres = malloc(sizeof(Nfs__ReadLinkRes));
    int status = nfs_procedure_5_read_from_symbolic_link(rpc_connection_context, *symlink_fhandle, readlinkres);
    if(status != 0) {
        free(readlinkres);
        cr_fatal("NFSPROC_READLINK failed - status %d\n", status);
    }

    cr_assert_not_null(readlinkres);

    return readlinkres;
}

/*
* Given the Nfs__FHandle of a symbolic link, calls NFSPROC_READLINK to read the pathname contained inside that
* symbolic link. Checks that the pathname read from this symbolic link is equal to the given null-terminated string
* 'expected_target_path'.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__ReadLinkRes returned by READLINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__read_link_res__free_unpacked()'
* with the obtained ReadLinkRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__ReadLinkRes
* and always call 'nfs__read_link_res__free_unpacked()' on it at some point.
*/
Nfs__ReadLinkRes *read_from_symbolic_link_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, char *expected_target_path) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadLinkRes *readlinkres = read_from_symbolic_link(rpc_connection_context, file_fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadLinkRes
    cr_assert_not_null(readlinkres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(readlinkres->nfs_status->stat);
    cr_assert_eq(readlinkres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(readlinkres->body_case, NFS__READ_LINK_RES__BODY_DATA);
    cr_assert_not_null(readlinkres->data);

    // validate read content
    cr_assert_not_null(readlinkres->data->path);
    char *read_content = readlinkres->data->path;
    
    cr_assert(strcmp(read_content, expected_target_path) == 0); // compare the target path we read from the symlink and the expected target path

    return readlinkres;
}

/*
* Given the Nfs__FHandle of a symbolic link, calls NFSPROC_READLINK to read the pathname contained inside that
* symbolic link.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void read_from_symbolic_link_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadLinkRes *readlinkres = read_from_symbolic_link(rpc_connection_context, file_fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadLinkRes
    cr_assert_not_null(readlinkres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(readlinkres->nfs_status->stat);
    cr_assert_eq(readlinkres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(readlinkres->body_case, NFS__READ_LINK_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readlinkres->default_case);

    nfs__read_link_res__free_unpacked(readlinkres, NULL);
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
Nfs__ReadRes *read_from_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count) {
    Nfs__ReadArgs readargs = NFS__READ_ARGS__INIT;
    readargs.file = file_fhandle;
    readargs.offset = offset;
    readargs.count = byte_count;
    readargs.totalcount = 0; // unused

    Nfs__ReadRes *readres = malloc(sizeof(Nfs__ReadRes));
    int status = nfs_procedure_6_read_from_file(rpc_connection_context, readargs, readres);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__ReadRes *read_from_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__FAttr *attributes_before_read, uint32_t expected_read_size, uint8_t *expected_read_content) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadRes *readres = read_from_file(rpc_connection_context, file_fhandle, offset, byte_count);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadRes
    cr_assert_not_null(readres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(readres->nfs_status->stat);
    cr_assert_eq(readres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(readres->body_case, NFS__READ_RES__BODY_READRESBODY);
    cr_assert_not_null(readres->readresbody);

    // validate attributes
    cr_assert_not_null(readres->readresbody->attributes);
    Nfs__FAttr *read_fattr = readres->readresbody->attributes;
    validate_fattr(read_fattr, attributes_before_read->nfs_ftype->ftype);
    check_equal_fattr(attributes_before_read, read_fattr);
    // this is a famous problem - atime is going to be flushed to disk by the kernel only after 24hrs, we can't synchronously update it - so we only check that atime_before <= atime_after
    cr_assert(compare_time(attributes_before_read->atime, read_fattr->atime) <= 0, "Access time before read is %ld sec %ld nanosec and is larger than access time after read %ld sec %ld nanosec\n", attributes_before_read->atime->seconds, attributes_before_read->atime->useconds, read_fattr->atime->seconds, read_fattr->atime->useconds);   // file was accessed
    cr_assert(compare_time(attributes_before_read->mtime, read_fattr->mtime) <= 0);   // file was not modified
    cr_assert(compare_time(attributes_before_read->ctime, read_fattr->ctime) <= 0);

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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void read_from_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadRes *readres = read_from_file(rpc_connection_context, file_fhandle, offset, byte_count);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadRes
    cr_assert_not_null(readres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(readres->nfs_status->stat);
    cr_assert_eq(readres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__AttrStat *write_to_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer) {
    Nfs__WriteArgs writeargs = NFS__WRITE_ARGS__INIT;
    writeargs.file = file_fhandle;
    writeargs.beginoffset = 0; // unused
    writeargs.offset = offset;
    writeargs.totalcount = 0;  // unused

    writeargs.nfsdata.data = source_buffer;
    writeargs.nfsdata.len = byte_count;

    Nfs__AttrStat *attrstat = malloc(sizeof(Nfs__AttrStat));
    int status = nfs_procedure_8_write_to_file(rpc_connection_context, writeargs, attrstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__AttrStat *write_to_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__FType ftype) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = write_to_file(rpc_connection_context, file_fhandle, offset, byte_count, source_buffer);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate AttrStat
    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void write_to_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *file_fhandle, uint32_t offset, uint32_t byte_count, uint8_t *source_buffer, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__AttrStat *attrstat = write_to_file(rpc_connection_context, file_fhandle, offset, byte_count, source_buffer);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(attrstat->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(attrstat->nfs_status->stat);
    cr_assert_eq(attrstat->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__DirOpRes *create_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__SAttr *sattr) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;
    createargs.attributes = sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_9_create_file(rpc_connection_context, createargs, diropres);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__DirOpRes *create_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) { 
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;
    
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = create_file(rpc_connection_context, directory_fhandle, filename, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void create_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, size_t size, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = size;
    sattr.atime = atime;
    sattr.mtime = mtime;

    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = create_file(rpc_connection_context, directory_fhandle, filename, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__NfsStat *remove_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_10_remove_file(rpc_connection_context, diropargs, nfsstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__NfsStat *remove_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = remove_file(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_REMOVE to delete the file with the given filename
* inside the given directory.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void remove_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = remove_file(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

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
Nfs__NfsStat *rename_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename) {
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
    int status = nfs_procedure_11_rename_file(rpc_connection_context, renameargs, nfsstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__NfsStat *rename_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = rename_file(rpc_connection_context, from_directory_fhandle, from_filename, to_directory_fhandle, to_filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle's of 'from' and 'to' directories, and Nfs__FileName's of 'from' and 'to' files/directories
* inside the 'from' and 'to' directores, calls NFSPROC_RENAME to rename the file with the given 'from' filename in the
* 'from' directory to the 'to' directory with the 'to' filename.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void rename_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *from_directory_fhandle, char *from_filename, Nfs__FHandle *to_directory_fhandle, char *to_filename, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = rename_file(rpc_connection_context, from_directory_fhandle, from_filename, to_directory_fhandle, to_filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LINK to create a hard link with the given filename
* inside this given directory, pointing to file given by 'target_file' NFS fhandle.
* 
* Returns the Nfs__NfsStat returned by LINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *create_link_to_file(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *target_file, Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__LinkArgs linkargs = NFS__LINK_ARGS__INIT;
    linkargs.from = target_file;
    linkargs.to = &diropargs;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_12_create_link_to_file(rpc_connection_context, linkargs, nfsstat);
    if(status != 0) {
        free(nfsstat);
        cr_fatal("NFSPROC_LINK failed - status %d\n", status);
    }

    cr_assert_not_null(nfsstat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LINK to create a hard link with the given filename
* inside this given directory, pointing to file given by 'target_file' NFS fhandle.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__NfsStat returned by LINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *create_link_to_file_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *target_file, Nfs__FHandle *directory_fhandle, char *filename) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = create_link_to_file(rpc_connection_context, target_file, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_LINK to create a hard link with the given filename
* inside this given directory, pointing to file given by 'target_file' NFS fhandle.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void create_link_to_file_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *target_file, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = create_link_to_file(rpc_connection_context, target_file, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    nfs__nfs_stat__free_unpacked(nfsstat, NULL);
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_SYMLINK to create a symbolic link with the given filename
* inside this given directory, pointing to file given by 'target' absolute path.
* 
* Returns the Nfs__NfsStat returned by SYMLINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *create_symbolic_link(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Path *target) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__SAttr sattr = NFS__SATTR__INIT;    // Unix-like NFS servers ignore 'attributes' for symbolic links, so just put -1 everywhere
    sattr.mode = -1;
    sattr.uid = sattr.gid = -1;
    sattr.size = -1;
    Nfs__TimeVal atime = NFS__TIME_VAL__INIT, mtime = NFS__TIME_VAL__INIT;
    atime.seconds = atime.useconds = mtime.seconds = mtime.useconds = -1;
    sattr.atime = &atime;
    sattr.mtime = &mtime;

    Nfs__SymLinkArgs symlinkargs = NFS__SYM_LINK_ARGS__INIT;
    symlinkargs.from = &diropargs;
    symlinkargs.to = target;
    symlinkargs.attributes = &sattr;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_13_create_symbolic_link(rpc_connection_context, symlinkargs, nfsstat);
    if(status != 0) {
        free(nfsstat);
        cr_fatal("NFSPROC_SYMLINK failed - status %d\n", status);
    }

    cr_assert_not_null(nfsstat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_SYMLINK to create a symbolic link with the given filename
* inside this given directory, pointing to file given by 'target' absolute path.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
* 
* Returns the Nfs__NfsStat returned by SYMLINK procedure.
*
* The user of this function takes on the responsibility to call 'nfs__nfs_stat__free_unpacked()'
* with the obtained NfsStat.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__NfsStat
* and always call 'nfs__nfs_stat__free_unpacked()' on it at some point.
*/
Nfs__NfsStat *create_symbolic_link_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Path *target) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = create_symbolic_link(rpc_connection_context, directory_fhandle, filename, target);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_SYMLINK to create a symbolic link with the given filename
* inside this given directory, pointing to file given by 'target' absolute path.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void create_symbolic_link_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Path *target, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = create_symbolic_link(rpc_connection_context, directory_fhandle, filename, target);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

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
Nfs__DirOpRes *create_directory(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__SAttr *sattr) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__CreateArgs createargs = NFS__CREATE_ARGS__INIT;
    createargs.where = &diropargs;
    createargs.attributes = sattr;

    Nfs__DirOpRes *diropres = malloc(sizeof(Nfs__DirOpRes));
    int status = nfs_procedure_14_create_directory(rpc_connection_context, createargs, diropres);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__DirOpRes *create_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__FType ftype) { 
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = -1;    // we do not (can not) set size attribute for directories
    sattr.atime = atime;
    sattr.mtime = mtime;
    
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = create_directory(rpc_connection_context, directory_fhandle, filename, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void create_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, mode_t mode, uid_t uid, uid_t gid, Nfs__TimeVal *atime, Nfs__TimeVal *mtime, Nfs__Stat non_nfs_ok_status) {
    Nfs__SAttr sattr = NFS__SATTR__INIT;
    sattr.mode = mode;
    sattr.uid = uid;
    sattr.gid = gid;
    sattr.size = -1;
    sattr.atime = atime;
    sattr.mtime = mtime;

    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__DirOpRes *diropres = create_directory(rpc_connection_context, directory_fhandle, filename, &sattr);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(diropres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(diropres->nfs_status->stat);
    cr_assert_eq(diropres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
Nfs__NfsStat *remove_directory(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename) {
    Nfs__FileName file_name = NFS__FILE_NAME__INIT;
    file_name.filename = filename;

    Nfs__DirOpArgs diropargs = NFS__DIR_OP_ARGS__INIT;
    diropargs.dir = directory_fhandle;
    diropargs.name = &file_name;

    Nfs__NfsStat *nfsstat = malloc(sizeof(Nfs__NfsStat));
    int status = nfs_procedure_15_remove_directory(rpc_connection_context, diropargs, nfsstat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__NfsStat *remove_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = remove_directory(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

    return nfsstat;
}

/*
* Given the Nfs__FHandle of a directory, calls NFSPROC_RMDIR to delete the directory with the given 
* filename inside the given directory.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void remove_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, char *filename, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__NfsStat *nfsstat = remove_directory(rpc_connection_context, directory_fhandle, filename);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(nfsstat->stat);
    cr_assert_eq(nfsstat->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);

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
Nfs__ReadDirRes *read_from_directory(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count) {
    Nfs__NfsCookie nfs_cookie = NFS__NFS_COOKIE__INIT;
    nfs_cookie.value = cookie;

    Nfs__ReadDirArgs readdirargs = NFS__READ_DIR_ARGS__INIT;
    readdirargs.dir = directory_fhandle;
    readdirargs.cookie = &nfs_cookie;
    readdirargs.count = byte_count;

    Nfs__ReadDirRes *readdirres = malloc(sizeof(Nfs__ReadDirRes));
    int status = nfs_procedure_16_read_from_directory(rpc_connection_context, readdirargs, readdirres);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
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
Nfs__ReadDirRes *read_from_directory_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, int expected_number_of_entries, char *expected_filenames[]) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadDirRes *readdirres = read_from_directory(rpc_connection_context, directory_fhandle, cookie, byte_count);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadDirRes
    cr_assert_not_null(readdirres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(readdirres->nfs_status->stat);
    cr_assert_eq(readdirres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
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
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void read_from_directory_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle *directory_fhandle, uint64_t cookie, uint32_t byte_count, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__ReadDirRes *readdirres = read_from_directory(rpc_connection_context, directory_fhandle, cookie, byte_count);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate ReadDirRes
    cr_assert_not_null(readdirres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(readdirres->nfs_status->stat);
    cr_assert_eq(readdirres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(readdirres->body_case, NFS__READ_DIR_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(readdirres->default_case);

    nfs__read_dir_res__free_unpacked(readdirres, NULL);
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_STATFS to get filesystem attributes.
* 
* Returns the Nfs__StatFsRes returned by STATFS procedure.
*
* The user of this function takes on the responsibility to call 'nfs__stat_fs_res__free_unpacked()'
* with the obtained StatFsRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__StatFsRes
* and always call 'nfs__stat_fs_res__free_unpacked()' on it at some point.
*/
Nfs__StatFsRes *get_filesystem_attributes(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle) {
    Nfs__StatFsRes *statfsres = malloc(sizeof(Nfs__StatFsRes));
    int status = nfs_procedure_17_get_filesystem_attributes(rpc_connection_context, fhandle, statfsres);
    if(status != 0) {
        free(statfsres);
        cr_fatal("NFSPROC_STATFS failed - status %d\n", status);
    }

    cr_assert_not_null(statfsres);

    return statfsres;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_STATFS to get filesystem attributes.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming NFS__STAT__NFS_OK NFS status.
*
* Returns the Nfs__StatFsRes returned by STATFS procedure.
*
* The user of this function takes on the responsibility to call 'nfs__stat_fs_res__free_unpacked()'
* with the obtained StatFsRes.
* This function either terminates the program (in case an assertion fails) or successfuly executes -
* so the user of this function should always assume this function returns a valid non-NULL Nfs__StatFsRes
* and always call 'nfs__stat_fs_res__free_unpacked()' on it at some point.
*/
Nfs__StatFsRes *get_filesystem_attributes_success(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__StatFsRes *statfsres = get_filesystem_attributes(rpc_connection_context, fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    // validate StatFsRes
    cr_assert_not_null(statfsres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(NFS__STAT__NFS_OK), *found_nfs_stat = nfs_stat_to_string(statfsres->nfs_status->stat);
    cr_assert_eq(statfsres->nfs_status->stat, NFS__STAT__NFS_OK, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(statfsres->body_case, NFS__STAT_FS_RES__BODY_FS_INFO);

    // validate FsInfo
    cr_assert_not_null(statfsres->fs_info);

    return statfsres;
}

/*
* Given the Nfs__FHandle of a file or a directory, calls NFSPROC_STATFS to get filesystem attributes.
*
* Uses the given RpcConnectionContext if it's not NULL, and if the given RpcConnectionContext is NULL
* it creates its own RpcConnectionContext with AUTH_SYS flavor and root uid.
*
* The procedure results are validated assuming a non-NFS__STAT__NFS_OK NFS status, given in argument 'non_nfs_ok_status'.
*/
void get_filesystem_attributes_fail(RpcConnectionContext *rpc_connection_context, Nfs__FHandle fhandle, Nfs__Stat non_nfs_ok_status) {
    int created_rpc_connection_context = 0;
    if(rpc_connection_context == NULL) {
        rpc_connection_context = create_test_rpc_connection_context(TEST_TRANSPORT_PROTOCOL);
        created_rpc_connection_context = 1;
    }
    Nfs__StatFsRes *statfsres = get_filesystem_attributes(rpc_connection_context, fhandle);
    if(created_rpc_connection_context) {
        free_rpc_connection_context(rpc_connection_context);
    }

    cr_assert_not_null(statfsres->nfs_status);
    char *expected_nfs_stat = nfs_stat_to_string(non_nfs_ok_status), *found_nfs_stat = nfs_stat_to_string(statfsres->nfs_status->stat);
    cr_assert_eq(statfsres->nfs_status->stat, non_nfs_ok_status, "Expected NfsStat %s but got %s", expected_nfs_stat, found_nfs_stat);
    free(expected_nfs_stat);
    free(found_nfs_stat);
    cr_assert_eq(statfsres->body_case, NFS__STAT_FS_RES__BODY_DEFAULT_CASE);
    cr_assert_not_null(statfsres->default_case);

    nfs__stat_fs_res__free_unpacked(statfsres, NULL);
}