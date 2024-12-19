#include "nfs_permissions.h"

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to mount the
* directory at absolute path 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_mount_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to mount a remote directory
    return 0;
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to get the
* attributes of the file/directory at absolute path 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_getattr_proc_permissions(char *absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to get the attributes of a file/directory
    return 0;
}

/*
* Given the SAttr, checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to 
* set the attributes of the file/directory at absolute path 'absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_setattr_proc_permissions(char *absolute_path, Nfs__SAttr *sattr, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    if(sattr == NULL) {
        return -1;
    }

    // only root user can change ownership
    if(sattr->uid != -1 || sattr->gid != -1) {
        return check_root_user(caller_uid, caller_gid);
    }

    // otherwise setting anything else requires write permission
    if(sattr->mode != -1 || sattr->size != -1 || sattr->atime->seconds != -1 || sattr->atime->useconds != -1 || sattr->mtime->seconds != -1 || sattr->mtime->useconds != -1) {
        return check_write_permission(absolute_path, caller_uid, caller_gid);
    }

    return 0;
}

/*
* Given the absolute path of the containing directory, checks if the caller given by 'caller_uid', 
* 'caller_gid' has correct permissions to lookup a file/directory inside that containing directory.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_lookup_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_execute_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to read from
* the symbolic link at 'symlink_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_readlink_proc_permissions(char *symlink_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_read_permission(symlink_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to read
* the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_read_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_read_permission(file_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to write to
* the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_write_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(file_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a file 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_create_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to remove a file 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_remove_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to move a file 
* inside the directory at 'from_directory_absolute_path' to directory at 'to_directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_rename_proc_permissions(char *from_directory_absolute_path, char *to_directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    int stat = check_write_permission(from_directory_absolute_path, caller_uid, caller_gid);
    if(stat < 0) {
        return -1;
    }
    if(stat == 1) {
        return 1;
    }

    return check_write_permission(to_directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a
* symbolic link inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_symlink_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to create a
* directory inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_mkdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to remove a directory 
* inside the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_rmdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_write_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to read entries in
* the directory at 'directory_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_readdir_proc_permissions(char *directory_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    return check_read_permission(directory_absolute_path, caller_uid, caller_gid);
}

/*
* Checks if the caller given by 'caller_uid', 'caller_gid' has correct permissions to get attributes of
* the file system which contains the file at 'file_absolute_path'.
*
* Returns < 0 on failure.
* On success, returns 0 if the caller has the permissions and 1 if it does not have permissions.
*/
int check_statfs_proc_permissions(char *file_absolute_path, uid_t caller_uid, gid_t caller_gid) {
    // the root can do anything
    if(check_root_user(caller_uid, caller_uid) == 0) {
        return 0;
    }

    // no special permissions are needed to get filesystem attributes
    return 0;
}